#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/state.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/exceptions.h"

namespace hue4cpp {

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
    // For now, use N-UPnP discovery as it's simpler and doesn't require mDNS
    // In the future, we can combine this with mDNS discovery for better results
    return discoverNUPnP();
}

std::vector<Bridge> Bridge::discoverMDNS() {
    // TODO: Implement mDNS discovery
    return {};
}

std::vector<Bridge> Bridge::discoverNUPnP() {
    std::vector<Bridge> bridges;
    
    try {
        // Create HTTP client for discovery request
        HttpClient client;
        client.setTimeout(std::chrono::milliseconds(5000));
        
        // Query the Philips Hue discovery service
        // This is the official N-UPnP (Network UPnP) endpoint
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
            info.model_id = json_utils::getValueOr<std::string>(bridge_json, "modelid", "");
            
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
