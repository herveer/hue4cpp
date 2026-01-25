/**
 * @file discovery.cpp
 * @brief Implementation of bridge discovery methods
 */

#include "hue4cpp/bridge.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/exceptions.h"
#include <set>
#include <chrono>
#include <map>
#include <vector>
#include <cstring>

// mDNS library for service discovery
extern "C" {
#include <mdns.h>
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#endif

namespace hue4cpp {

namespace {

// Structure to hold discovered bridge information during mDNS scan
struct DiscoveredBridge {
    std::string service_name;     // From PTR record
    std::string hostname;          // From SRV record
    std::string ip_address;        // From A/AAAA record
    std::string id;                // From TXT record (bridgeid)
    uint16_t port = 443;          // From SRV record, default HTTPS port
    bool is_ipv6 = false;         // Track if this is an IPv6 address
};

// User data structure for mDNS callback
struct MDNSUserData {
    std::map<std::string, DiscoveredBridge> services;  // Map by service name
    std::map<std::string, std::string> hostname_to_ip;  // Map hostname to IP
    char name_buffer[256];
};

// mDNS callback function to process discovered services
//
// NOTE: This implementation uses a simplified heuristic for correlating mDNS records
// (PTR -> SRV -> A/AAAA -> TXT). It works correctly for single-bridge networks and
// most common scenarios, but may have issues in complex multi-bridge environments
// where multiple bridges respond simultaneously with interleaved records.
//
// A fully robust implementation would require:
// - Parsing and matching the full DNS query names from each record
// - More sophisticated correlation logic to handle out-of-order responses
// - Better handling of multiple responses for the same service
//
// Future improvement: Parse DNS names from records for precise correlation.
//
int mdns_callback(int sock, const struct sockaddr* from, size_t addrlen,
                  mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                  uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                  size_t name_offset, size_t name_length, size_t record_offset,
                  size_t record_length, void* user_data) {
    (void)sock;
    (void)from;
    (void)addrlen;
    (void)query_id;
    (void)rclass;
    (void)ttl;
    (void)name_offset;
    (void)name_length;
    
    if (!user_data) {
        return 0;
    }
    
    MDNSUserData* mdns_data = static_cast<MDNSUserData*>(user_data);
    
    // We're only interested in answer records
    if (entry != MDNS_ENTRYTYPE_ANSWER) {
        return 0;
    }
    
    // Handle different record types
    if (rtype == MDNS_RECORDTYPE_PTR) {
        // PTR record points to a service instance name
        mdns_string_t name = mdns_record_parse_ptr(data, size, record_offset, record_length,
                                                     mdns_data->name_buffer, sizeof(mdns_data->name_buffer));
        if (name.length > 0) {
            std::string service_name(name.str, name.length);
            // Create a new bridge entry for this service
            mdns_data->services[service_name] = DiscoveredBridge();
            mdns_data->services[service_name].service_name = service_name;
        }
    }
    else if (rtype == MDNS_RECORDTYPE_SRV) {
        // SRV record contains hostname and port
        mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length,
                                                        mdns_data->name_buffer, sizeof(mdns_data->name_buffer));
        if (srv.name.length > 0) {
            std::string hostname(srv.name.str, srv.name.length);
            // Find the service entry to update (look for one without a hostname)
            for (auto& entry : mdns_data->services) {
                if (entry.second.hostname.empty()) {
                    entry.second.hostname = hostname;
                    entry.second.port = srv.port;
                    break;
                }
            }
        }
    }
    else if (rtype == MDNS_RECORDTYPE_A) {
        // A record contains IPv4 address
        struct sockaddr_in addr;
        if (mdns_record_parse_a(data, size, record_offset, record_length, &addr)) {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
            
            // Store hostname to IP mapping
            // This will be used to match with services later
            std::string ip(ip_str);
            
            // Try to match with a service by hostname
            for (auto& entry : mdns_data->services) {
                if (entry.second.ip_address.empty() && !entry.second.hostname.empty()) {
                    // Simple heuristic: assume this IP belongs to the service we just saw
                    entry.second.ip_address = ip;
                    entry.second.is_ipv6 = false;
                    break;
                }
            }
        }
    }
    else if (rtype == MDNS_RECORDTYPE_AAAA) {
        // AAAA record contains IPv6 address
        struct sockaddr_in6 addr;
        if (mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr)) {
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &addr.sin6_addr, ip_str, INET6_ADDRSTRLEN);
            
            std::string ip(ip_str);
            
            // Try to match with a service by hostname
            // Only use IPv6 if IPv4 is not available
            for (auto& entry : mdns_data->services) {
                if (entry.second.ip_address.empty() && !entry.second.hostname.empty()) {
                    entry.second.ip_address = ip;
                    entry.second.is_ipv6 = true;
                    break;
                }
            }
        }
    }
    else if (rtype == MDNS_RECORDTYPE_TXT) {
        // TXT records contain bridge metadata like bridgeid and modelid
        mdns_record_txt_t txt_records[16];
        size_t txt_count = mdns_record_parse_txt(data, size, record_offset, record_length,
                                                   txt_records, sizeof(txt_records) / sizeof(mdns_record_txt_t));
        
        // Extract bridge ID from TXT records
        std::string bridge_id;
        for (size_t i = 0; i < txt_count; ++i) {
            std::string key(txt_records[i].key.str, txt_records[i].key.length);
            std::string value(txt_records[i].value.str, txt_records[i].value.length);
            
            if (key == "bridgeid") {
                bridge_id = value;
                break;
            }
        }
        
        // Assign to the most recently added service without an ID
        if (!bridge_id.empty()) {
            for (auto it = mdns_data->services.rbegin(); it != mdns_data->services.rend(); ++it) {
                if (it->second.id.empty()) {
                    it->second.id = bridge_id;
                    break;
                }
            }
        }
    }
    
    return 0;  // Continue processing
}

// Constants for discovery timeouts
constexpr std::chrono::milliseconds BRIDGE_VERIFY_TIMEOUT{5000};  // 5 seconds for verification
constexpr std::chrono::milliseconds BRIDGE_REACHABLE_TIMEOUT{3000}; // 3 seconds for quick reachability check

// Helper function to verify bridge by getting its configuration
bool verifyBridge(BridgeInfo& info) {
    try {
        HttpClient client;
        client.setVerifySsl(false);  // Hue bridges use self-signed certificates
        client.setTimeout(BRIDGE_VERIFY_TIMEOUT);
        
        std::string url = "https://" + info.ip_address + "/api/0/config";
        auto response = client.get(url);
        
        if (!response.isSuccess()) {
            return false;
        }
        
        auto json = json_utils::parse(response.body);
        
        // Extract additional information from the config response
        info.name = json_utils::getValueOr<std::string>(json, "name", "");
        info.sw_version = json_utils::getValueOr<std::string>(json, "swversion", "");
        info.model_id = json_utils::getValueOr<std::string>(json, "modelid", "");
        
        // Get bridge ID from config if not already set
        if (info.id.empty()) {
            info.id = json_utils::getValueOr<std::string>(json, "bridgeid", "");
        }
        
        return !info.id.empty();
        
    } catch (const std::exception&) {
        return false;
    }
}

} // anonymous namespace

// Implementation of discoverMDNS
std::vector<Bridge> Bridge::discoverMDNS() {
    std::vector<Bridge> result;
    
    // Open mDNS socket for IPv4
    int sock = mdns_socket_open_ipv4(nullptr);
    if (sock < 0) {
        // Failed to open socket, return empty result
        return result;
    }
    
    // Prepare query for Hue service
    const char* service_name = "_hue._tcp.local";
    size_t service_name_length = strlen(service_name);
    
    // Buffer for mDNS packets (must be 32-bit aligned)
    alignas(4) char buffer[2048];
    
    // Send mDNS query for Hue bridges
    int query_result = mdns_query_send(sock, MDNS_RECORDTYPE_PTR, 
                                        service_name, service_name_length,
                                        buffer, sizeof(buffer), 0);
    
    if (query_result < 0) {
        mdns_socket_close(sock);
        return result;
    }
    
    // Wait for responses (with timeout)
    MDNSUserData user_data;
    
    // Set up select for timeout
    fd_set readfs;
    struct timeval timeout;
    
    FD_ZERO(&readfs);
    FD_SET(sock, &readfs);
    timeout.tv_sec = 2;  // 2 second timeout
    timeout.tv_usec = 0;
    
#ifdef _WIN32
    int nfds = 0;  // Ignored on Windows
#else
    int nfds = sock + 1;  // POSIX requires highest fd + 1
#endif
    
    int select_result = select(nfds, &readfs, nullptr, nullptr, &timeout);
    
    if (select_result > 0) {
        // Receive responses
        mdns_query_recv(sock, buffer, sizeof(buffer), mdns_callback, &user_data, 0);
        
        // Give it a bit more time to receive all responses
        // Reset fd_set as select() modifies it on most UNIX systems (required for portable code)
        FD_ZERO(&readfs);
        FD_SET(sock, &readfs);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;  // 500ms
        select_result = select(nfds, &readfs, nullptr, nullptr, &timeout);
        if (select_result > 0) {
            mdns_query_recv(sock, buffer, sizeof(buffer), mdns_callback, &user_data, 0);
        }
    }
    
    // Close the socket
    mdns_socket_close(sock);
    
    // Convert discovered services to Bridge objects
    for (const auto& entry : user_data.services) {
        const DiscoveredBridge& discovered = entry.second;
        
        // Skip if we don't have an IP address
        if (discovered.ip_address.empty()) {
            continue;
        }
        
        BridgeInfo info;
        // Handle IPv6 addresses - add brackets for URL formatting
        if (discovered.is_ipv6) {
            info.ip_address = "[" + discovered.ip_address + "]";
        } else {
            info.ip_address = discovered.ip_address;
        }
        info.id = discovered.id;
        
        // Verify the bridge and get additional information
        if (verifyBridge(info)) {
            result.emplace_back(info);
        }
    }
    
    return result;
}

// Implementation of discoverNUPnP
std::vector<Bridge> Bridge::discoverNUPnP() {
    std::vector<Bridge> result;
    
    try {
        HttpClient client;
        auto response = client.get("https://discovery.meethue.com");
        
        if (!response.isSuccess()) {
            return result;  // Return empty vector on error
        }
        
        // Parse JSON response
        auto json = json_utils::parse(response.body);
        
        if (!json.is_array()) {
            return result;
        }
        
        // Extract bridge information from each entry
        for (const auto& entry : json) {
            BridgeInfo info;
            
            info.id = json_utils::getValueOr<std::string>(entry, "id", "");
            info.ip_address = json_utils::getValueOr<std::string>(entry, "internalipaddress", "");
            
            // Skip if missing required fields
            if (info.id.empty() || info.ip_address.empty()) {
                continue;
            }
            
            // Try to get optional fields
            info.name = json_utils::getValueOr<std::string>(entry, "name", "");
            
            // Verify the bridge and get full config
            if (verifyBridge(info)) {
                result.emplace_back(info);
            }
        }
        
    } catch (const std::exception&) {
        // Return empty vector on any error
    }
    
    return result;
}

// Implementation of combined discover
std::vector<Bridge> Bridge::discover() {
    std::vector<Bridge> result;
    std::set<std::string> seen_bridge_ids;
    
    // First, try mDNS discovery (preferred method)
    // Note: mDNS discovery is not yet implemented
    auto mdns_bridges = discoverMDNS();
    for (auto& bridge : mdns_bridges) {
        const auto& id = bridge.getInfo().id;
        if (!id.empty() && seen_bridge_ids.find(id) == seen_bridge_ids.end()) {
            seen_bridge_ids.insert(id);
            result.push_back(std::move(bridge));
        }
    }
    
    // Then, try N-UPnP discovery
    auto nupnp_bridges = discoverNUPnP();
    for (auto& bridge : nupnp_bridges) {
        const auto& id = bridge.getInfo().id;
        if (!id.empty() && seen_bridge_ids.find(id) == seen_bridge_ids.end()) {
            seen_bridge_ids.insert(id);
            result.push_back(std::move(bridge));
        }
    }
    
    return result;
}

// Implementation of isReachable
bool Bridge::isReachable() const {
    const auto& info = getInfo();
    if (info.ip_address.empty()) {
        return false;
    }
    
    try {
        HttpClient client;
        client.setVerifySsl(false);  // Hue bridges use self-signed certificates
        client.setTimeout(BRIDGE_REACHABLE_TIMEOUT);
        
        std::string url = "https://" + info.ip_address + "/api/0/config";
        auto response = client.get(url);
        
        return response.isSuccess();
        
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace hue4cpp
