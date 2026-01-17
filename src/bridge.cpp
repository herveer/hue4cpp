#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/state.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/exceptions.h"

#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <condition_variable>

#ifdef _WIN32
// Windows: Use Bonjour/DNS-SD API
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dns_sd.h>
#else
// Linux/macOS: Use mdns library
#include <mdns.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

namespace hue4cpp {

// Helper structure to collect mDNS results
struct MDNSDiscoveryResult {
    std::string name;
    std::string ip_address;
    std::string bridge_id;
    std::string model_id;
    uint16_t port;
};

#ifdef _WIN32
// Windows DNS-SD implementation

struct DNSSDContext {
    std::vector<MDNSDiscoveryResult> results;
    std::mutex mutex;
    std::condition_variable cv;
    bool browse_finished = false;
    bool resolve_finished = false;
};

// DNS-SD Browse callback
static void DNSSD_API browse_callback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* serviceName,
    const char* regtype,
    const char* replyDomain,
    void* context) {
    
    (void)sdRef;
    (void)regtype;
    (void)replyDomain;
    
    if (errorCode != kDNSServiceErr_NoError) {
        return;
    }
    
    auto* ctx = static_cast<DNSSDContext*>(context);
    
    if (flags & kDNSServiceFlagsAdd) {
        // Found a service, now resolve it
        DNSServiceRef resolveRef;
        DNSServiceResolve(&resolveRef, 0, interfaceIndex,
                         serviceName, regtype, replyDomain,
                         nullptr, context);  // We'll handle resolution separately
        DNSServiceRefDeallocate(resolveRef);
    }
}

// DNS-SD Resolve callback
static void DNSSD_API resolve_callback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* fullname,
    const char* hosttarget,
    uint16_t port,
    uint16_t txtLen,
    const unsigned char* txtRecord,
    void* context) {
    
    (void)sdRef;
    (void)flags;
    (void)interfaceIndex;
    (void)hosttarget;
    
    if (errorCode != kDNSServiceErr_NoError) {
        return;
    }
    
    auto* ctx = static_cast<DNSSDContext*>(context);
    std::lock_guard<std::mutex> lock(ctx->mutex);
    
    MDNSDiscoveryResult result;
    result.name = fullname ? fullname : "";
    result.port = ntohs(port);
    
    // Parse TXT records for bridgeid and modelid
    if (txtLen > 0 && txtRecord) {
        uint16_t keyLen;
        const char* key;
        uint8_t valueLen;
        const void* value;
        
        const unsigned char* ptr = txtRecord;
        while (ptr < txtRecord + txtLen) {
            uint8_t len = *ptr++;
            if (len == 0) break;
            
            const char* equals = (const char*)memchr(ptr, '=', len);
            if (equals) {
                keyLen = equals - (const char*)ptr;
                key = (const char*)ptr;
                valueLen = len - keyLen - 1;
                value = equals + 1;
                
                std::string keyStr(key, keyLen);
                std::string valueStr((const char*)value, valueLen);
                
                if (keyStr == "bridgeid") {
                    result.bridge_id = valueStr;
                } else if (keyStr == "modelid") {
                    result.model_id = valueStr;
                }
            }
            ptr += len;
        }
    }
    
    ctx->results.push_back(result);
}

// DNS-SD GetAddrInfo callback
static void DNSSD_API getaddr_callback(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char* hostname,
    const struct sockaddr* address,
    uint32_t ttl,
    void* context) {
    
    (void)sdRef;
    (void)flags;
    (void)interfaceIndex;
    (void)hostname;
    (void)ttl;
    
    if (errorCode != kDNSServiceErr_NoError || !address) {
        return;
    }
    
    auto* ctx = static_cast<DNSSDContext*>(context);
    std::lock_guard<std::mutex> lock(ctx->mutex);
    
    char addr_str[INET6_ADDRSTRLEN];
    if (address->sa_family == AF_INET) {
        auto* addr_in = (const struct sockaddr_in*)address;
        inet_ntop(AF_INET, &addr_in->sin_addr, addr_str, sizeof(addr_str));
        
        // Update the last result with IP address
        if (!ctx->results.empty()) {
            ctx->results.back().ip_address = addr_str;
        }
    }
}

#else
// Linux/macOS mdns library implementation

// Callback for mDNS query responses
static int mdns_query_callback(int sock, const struct sockaddr* from, size_t addrlen,
                                mdns_entry_type_t entry, uint16_t query_id,
                                uint16_t rtype, uint16_t rclass, uint32_t ttl,
                                const void* data, size_t size, size_t name_offset,
                                size_t name_length, size_t record_offset,
                                size_t record_length, void* user_data) {
    (void)sock;
    (void)from;
    (void)addrlen;
    (void)query_id;
    (void)rclass;
    (void)ttl;
    (void)name_offset;
    (void)name_length;
    
    auto* results = static_cast<std::vector<MDNSDiscoveryResult>*>(user_data);
    
    char namebuffer[256];
    char addrbuffer[64];
    
    if (entry == MDNS_ENTRYTYPE_ANSWER) {
        if (rtype == MDNS_RECORDTYPE_PTR) {
            // PTR record - service instance name
            mdns_record_parse_ptr(data, size, record_offset, record_length,
                                  namebuffer, sizeof(namebuffer));
        } else if (rtype == MDNS_RECORDTYPE_SRV) {
            // SRV record - contains hostname and port
            mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset,
                                                          record_length,
                                                          namebuffer, sizeof(namebuffer));
            
            MDNSDiscoveryResult result;
            result.name = std::string(namebuffer);
            result.port = srv.port;
            
            // Extract bridge ID from service name if present
            // Format: "Philips Hue - XXXXXX._hue._tcp.local"
            std::string service_name(namebuffer);
            size_t pos = service_name.find("Philips Hue - ");
            if (pos != std::string::npos) {
                size_t id_start = pos + 14; // Length of "Philips Hue - "
                size_t id_end = service_name.find('.', id_start);
                if (id_end != std::string::npos) {
                    std::string last6 = service_name.substr(id_start, id_end - id_start);
                    // Bridge ID would need to be constructed, but we'll get it from TXT records
                }
            }
            
            // Store partial result
            results->push_back(result);
        } else if (rtype == MDNS_RECORDTYPE_A) {
            // A record - IPv4 address
            struct sockaddr_in addr;
            mdns_record_parse_a(data, size, record_offset, record_length, &addr);
            
            // Convert IPv4 address to string
            if (inet_ntop(AF_INET, &(addr.sin_addr), addrbuffer, sizeof(addrbuffer))) {
                // Find existing result and add IP
                if (!results->empty()) {
                    results->back().ip_address = std::string(addrbuffer);
                }
            }
        } else if (rtype == MDNS_RECORDTYPE_AAAA) {
            // AAAA record - IPv6 address
            struct sockaddr_in6 addr;
            mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr);
            
            // Convert IPv6 address to string
            if (inet_ntop(AF_INET6, &(addr.sin6_addr), addrbuffer, sizeof(addrbuffer))) {
                // We prefer IPv4, so only use IPv6 if no IPv6 is available
                if (!results->empty() && results->back().ip_address.empty()) {
                    results->back().ip_address = std::string(addrbuffer);
                }
            }
        } else if (rtype == MDNS_RECORDTYPE_TXT) {
            // TXT record - contains bridgeid and modelid
            mdns_record_txt_t txt_records[16];
            size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length,
                                                  txt_records, sizeof(txt_records) / sizeof(mdns_record_txt_t));
            
            for (size_t itxt = 0; itxt < parsed; ++itxt) {
                std::string key(txt_records[itxt].key.str, txt_records[itxt].key.length);
                std::string value(txt_records[itxt].value.str, txt_records[itxt].value.length);
                
                if (!results->empty()) {
                    if (key == "bridgeid") {
                        results->back().bridge_id = value;
                    } else if (key == "modelid") {
                        results->back().model_id = value;
                    }
                }
            }
        }
    }
    
    return 0;
}

// Open a socket for mDNS
static int mdns_open_socket() {
    int sock = mdns_socket_open_ipv4(nullptr);
    if (sock < 0)
        return -1;
    return sock;
}

#endif

// Bridge::Impl definition
class Bridge::Impl {
public:
    BridgeInfo info;
    std::string auth_key;
    std::unique_ptr<StateManager> state_manager;
    std::unique_ptr<HttpClient> http_client;
    
    Impl() : state_manager(std::make_unique<StateManager>()),
             http_client(std::make_unique<HttpClient>()) {}
    explicit Impl(const BridgeInfo& bridge_info)
        : info(bridge_info), 
          state_manager(std::make_unique<StateManager>()),
          http_client(std::make_unique<HttpClient>()) {}
};

// Bridge implementation
Bridge::Bridge() : pImpl(std::make_unique<Impl>()) {}

Bridge::Bridge(const BridgeInfo& info) : pImpl(std::make_unique<Impl>(info)) {}

Bridge::~Bridge() = default;

Bridge::Bridge(Bridge&&) noexcept = default;
Bridge& Bridge::operator=(Bridge&&) noexcept = default;

std::vector<Bridge> Bridge::discover() {
    // Try mDNS discovery first (preferred local discovery method)
    std::vector<Bridge> bridges = discoverMDNS();
    
    // If mDNS didn't find anything, try remote discovery as fallback
    if (bridges.empty()) {
        bridges = discoverRemote();
    }
    
    return bridges;
}

std::vector<Bridge> Bridge::discoverMDNS() {
    std::vector<Bridge> bridges;
    
    try {
#ifdef _WIN32
        // Windows implementation using DNS-SD (Bonjour) API
        DNSSDContext context;
        DNSServiceRef browseRef = nullptr;
        
        // Browse for _hue._tcp services
        DNSServiceErrorType err = DNSServiceBrowse(
            &browseRef,
            0,  // flags
            0,  // interface index (0 = all interfaces)
            "_hue._tcp",
            nullptr,  // domain (nullptr = default domains)
            browse_callback,
            &context);
        
        if (err != kDNSServiceErr_NoError) {
            return bridges;
        }
        
        // Process events for 3 seconds
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(3);
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            int dns_sd_fd = DNSServiceRefSockFD(browseRef);
            if (dns_sd_fd < 0) {
                break;
            }
            
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(dns_sd_fd, &readfds);
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;  // 100ms
            
            int result = select(dns_sd_fd + 1, &readfds, nullptr, nullptr, &tv);
            if (result > 0 && FD_ISSET(dns_sd_fd, &readfds)) {
                DNSServiceProcessResult(browseRef);
            }
        }
        
        DNSServiceRefDeallocate(browseRef);
        
        // Convert DNS-SD results to Bridge objects
        std::lock_guard<std::mutex> lock(context.mutex);
        for (const auto& result : context.results) {
            // Skip incomplete results
            if (result.ip_address.empty() || result.bridge_id.empty()) {
                continue;
            }
            
            BridgeInfo info;
            info.ip_address = result.ip_address;
            info.id = result.bridge_id;
            info.name = result.name;
            info.model_id = result.model_id;
            
            bridges.push_back(Bridge(info));
        }
#else
        // Linux/macOS implementation using mdns library
        std::vector<MDNSDiscoveryResult> mdns_results;
        
        // Open mDNS socket
        int sock = mdns_open_socket();
        if (sock < 0) {
            // Failed to open socket, return empty vector
            return bridges;
        }
        
        // Query for Hue bridges using the registered mDNS service name
        // Service: _hue._tcp.local
        const char* service = "_hue._tcp.local";
        
        // Send mDNS query
        if (mdns_query_send(sock, MDNS_RECORDTYPE_PTR,
                           service, strlen(service),
                           nullptr, 0, 0) < 0) {
            mdns_socket_close(sock);
            return bridges;
        }
        
        // Wait for responses (3 seconds timeout)
        // mDNS responses may come from multiple bridges
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(3);
        
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            
            fd_set readfs;
            FD_ZERO(&readfs);
            FD_SET(sock, &readfs);
            
            int nfds = sock + 1;
            if (select(nfds, &readfs, nullptr, nullptr, &tv) > 0) {
                if (FD_ISSET(sock, &readfs)) {
                    // Receive and parse mDNS response
                    mdns_query_recv(sock, nullptr, 0, mdns_query_callback,
                                   &mdns_results, 1);
                }
            }
        }
        
        // Close socket
        mdns_socket_close(sock);
        
        // Convert mDNS results to Bridge objects
        for (const auto& result : mdns_results) {
            // Skip incomplete results
            if (result.ip_address.empty() || result.bridge_id.empty()) {
                continue;
            }
            
            BridgeInfo info;
            info.ip_address = result.ip_address;
            info.id = result.bridge_id;
            info.name = result.name;
            info.model_id = result.model_id;
            
            bridges.push_back(Bridge(info));
        }
#endif
        
    } catch (const std::exception&) {
        // If anything goes wrong, return what we have collected so far
        return bridges;
    }
    
    return bridges;
}

std::vector<Bridge> Bridge::discoverRemote() {
    std::vector<Bridge> bridges;
    
    try {
        // Create HTTP client for discovery request
        HttpClient client;
        client.setTimeout(std::chrono::milliseconds(5000));
        
        // Query the Philips Hue cloud discovery endpoint
        // This endpoint returns bridges on the same network (same public IP)
        // The Hue bridge periodically updates the cloud with its local IP
        auto response = client.get("https://discovery.meethue.com/");
        
        if (!response.isSuccess()) {
            // Discovery failed, but we don't throw - just return empty list
            return bridges;
        }
        
        // Parse the JSON response
        auto json = json_utils::parse(response.body);
        
        // The response should be an array of bridge objects
        if (!json.is_array()) {
            return bridges;
        }
        
        // Parse each bridge in the response
        for (const auto& bridge_json : json) {
            if (!bridge_json.is_object()) {
                continue;
            }
            
            // Extract bridge information
            auto id = json_utils::getValue<std::string>(bridge_json, "id");
            auto ip = json_utils::getValue<std::string>(bridge_json, "internalipaddress");
            
            // Both id and ip are required
            if (!id || !ip) {
                continue;
            }
            
            // Create BridgeInfo and Bridge
            BridgeInfo info(ip.value(), id.value());
            
            // Optional fields
            info.name = json_utils::getValueOr<std::string>(bridge_json, "name", "");
            // Note: modelid is not always present in the discovery response
            
            bridges.push_back(Bridge(info));
        }
        
    } catch (const std::exception&) {
        // If anything goes wrong, return empty list
        // Discovery should be robust and not throw
        return bridges;
    }
    
    return bridges;
}

Result<std::string> Bridge::authenticate(const std::string& app_name,
                                         const std::string& device_name) {
    // TODO: Implement authentication
    return Result<std::string>(ErrorCode::UnknownError, "Not implemented");
}

void Bridge::setAuthenticationKey(const std::string& key) {
    pImpl->auth_key = key;
}

bool Bridge::isAuthenticated() const {
    return !pImpl->auth_key.empty();
}

std::vector<Light> Bridge::getLights() {
    // TODO: Implement get lights
    return {};
}

std::optional<Light> Bridge::getLight(const std::string& light_id) {
    // TODO: Implement get light
    return std::nullopt;
}

const BridgeInfo& Bridge::getInfo() const {
    return pImpl->info;
}

StateManager& Bridge::getStateManager() {
    return *pImpl->state_manager;
}

bool Bridge::isReachable() const {
    // Check if we have a valid IP address
    if (pImpl->info.ip_address.empty()) {
        return false;
    }
    
    try {
        // Try to reach the bridge by accessing a public endpoint
        // We use /api/0/config which doesn't require authentication
        std::string url = "https://" + pImpl->info.ip_address + "/api/0/config";
        
        // Configure HTTP client with short timeout for reachability check
        pImpl->http_client->setTimeout(std::chrono::milliseconds(3000));
        pImpl->http_client->setVerifySsl(false); // Local bridges use self-signed certs
        
        auto response = pImpl->http_client->get(url);
        
        // Bridge is reachable if we get any response in the 2xx or 4xx range
        // (4xx means the bridge responded, even if our request wasn't perfect)
        return response.status_code >= 200 && response.status_code < 500;
        
    } catch (const std::exception&) {
        // Any exception means the bridge is not reachable
        return false;
    }
}

} // namespace hue4cpp
