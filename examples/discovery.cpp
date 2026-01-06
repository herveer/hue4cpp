#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <iomanip>

/**
 * @brief Discovery example showing how to find Hue bridges
 * 
 * This example demonstrates:
 * - Different discovery methods (mDNS and N-UPnP)
 * - Displaying bridge information
 * - Checking bridge reachability
 */

void printBridgeInfo(const hue4cpp::BridgeInfo& info) {
    std::cout << "  Bridge ID:    " << info.id << std::endl;
    std::cout << "  Name:         " << info.name << std::endl;
    std::cout << "  IP Address:   " << info.ip_address << std::endl;
    std::cout << "  Model:        " << info.model_id << std::endl;
    std::cout << "  SW Version:   " << info.sw_version << std::endl;
}

int main() {
    std::cout << "hue4cpp - Bridge Discovery Example" << std::endl;
    std::cout << "Version: " << hue4cpp::Version::STRING << std::endl;
    std::cout << std::endl;
    
    // Try all discovery methods
    std::cout << "=== Discovering bridges using all methods ===" << std::endl;
    auto all_bridges = hue4cpp::Bridge::discover();
    
    if (all_bridges.empty()) {
        std::cout << "No bridges found using automatic discovery." << std::endl;
    } else {
        std::cout << "Found " << all_bridges.size() << " bridge(s):" << std::endl;
        for (size_t i = 0; i < all_bridges.size(); ++i) {
            std::cout << std::endl;
            std::cout << "Bridge " << (i + 1) << ":" << std::endl;
            printBridgeInfo(all_bridges[i].getInfo());
            
            std::cout << "  Reachable:    " 
                     << (all_bridges[i].isReachable() ? "Yes" : "No") 
                     << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== Trying mDNS discovery ===" << std::endl;
    auto mdns_bridges = hue4cpp::Bridge::discoverMDNS();
    std::cout << "Found " << mdns_bridges.size() << " bridge(s) via mDNS" << std::endl;
    
    std::cout << std::endl;
    std::cout << "=== Trying N-UPnP discovery ===" << std::endl;
    auto nupnp_bridges = hue4cpp::Bridge::discoverNUPnP();
    std::cout << "Found " << nupnp_bridges.size() << " bridge(s) via N-UPnP" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Discovery example completed!" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: To manually connect to a bridge, create a BridgeInfo" << std::endl;
    std::cout << "      with the bridge's IP address and use it to construct a Bridge." << std::endl;
    
    return 0;
}
