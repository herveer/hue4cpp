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

namespace hue4cpp {

namespace {

// Helper function to verify bridge by getting its configuration
bool verifyBridge(BridgeInfo& info) {
    try {
        HttpClient client;
        client.setVerifySsl(false);  // Hue bridges use self-signed certificates
        client.setTimeout(std::chrono::milliseconds(5000));  // 5 second timeout
        
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
    // TODO: mDNS discovery will be implemented in a future update
    // 
    // mDNS (Multicast DNS) is the preferred method for discovering Hue bridges
    // on the local network. The Hue bridge advertises itself via the _hue._tcp
    // service type, including TXT records with bridgeid and modelid.
    //
    // Implementation plan:
    // 1. Integrate cross-platform mDNS library (e.g., mjansson/mdns)
    // 2. Send DNS-SD query for "_hue._tcp.local"
    // 3. Parse PTR, SRV, TXT, and A records from responses
    // 4. Extract bridge ID, model ID, hostname, port, and IP address
    // 5. Verify discovered bridges using GET /api/0/config
    //
    // For now, use discoverNUPnP() or manual bridge configuration as alternatives.
    //
    return {};
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
        client.setTimeout(std::chrono::milliseconds(3000));  // 3 second timeout
        
        std::string url = "https://" + info.ip_address + "/api/0/config";
        auto response = client.get(url);
        
        return response.isSuccess();
        
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace hue4cpp
