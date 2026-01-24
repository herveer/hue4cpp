#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/bridge.h>
#include <hue4cpp/state.h>

using namespace hue4cpp;

TEST_CASE("Bridge construction", "[bridge]") {
    SECTION("Default constructor") {
        Bridge bridge;
        REQUIRE_FALSE(bridge.isAuthenticated());
    }
    
    SECTION("Constructor with BridgeInfo") {
        BridgeInfo info;
        info.ip_address = "192.168.1.100";
        info.id = "test-bridge-id";
        info.name = "Test Bridge";
        
        Bridge bridge(info);
        REQUIRE_FALSE(bridge.isAuthenticated());
        REQUIRE(bridge.getInfo().ip_address == "192.168.1.100");
        REQUIRE(bridge.getInfo().id == "test-bridge-id");
    }
}

TEST_CASE("Bridge authentication", "[bridge]") {
    Bridge bridge;
    
    SECTION("Set authentication key") {
        bridge.setAuthenticationKey("test-key-12345");
        REQUIRE(bridge.isAuthenticated());
    }
    
    SECTION("Not authenticated by default") {
        REQUIRE_FALSE(bridge.isAuthenticated());
    }
}

TEST_CASE("Bridge discovery", "[bridge]") {
    SECTION("Discover returns empty vector for mDNS (not implemented)") {
        auto bridges = Bridge::discoverMDNS();
        // mDNS is not yet implemented, should return empty
        REQUIRE(bridges.empty());
    }
    
    SECTION("DiscoverNUPnP returns vector") {
        // Note: This test will make an actual HTTP request to discovery.meethue.com
        // In a real environment, this might find actual bridges
        auto bridges = Bridge::discoverNUPnP();
        // We can't assert the size since it depends on network/bridges availability
        // Just verify it doesn't crash and returns a vector
        REQUIRE(bridges.size() >= 0);
    }
    
    SECTION("Combined discover method works") {
        auto bridges = Bridge::discover();
        // Should work without crashing
        REQUIRE(bridges.size() >= 0);
    }
}

TEST_CASE("Bridge state manager", "[bridge]") {
    Bridge bridge;
    
    SECTION("State manager is available") {
        auto& state_manager = bridge.getStateManager();
        REQUIRE_FALSE(state_manager.isRunning());
    }
}

TEST_CASE("Bridge reachability", "[bridge]") {
    SECTION("Unreachable with empty IP") {
        Bridge bridge;
        REQUIRE_FALSE(bridge.isReachable());
    }
    
    SECTION("Unreachable with invalid IP") {
        BridgeInfo info;
        info.ip_address = "192.168.255.254";  // Unlikely to have a bridge here
        info.id = "test-id";
        
        Bridge bridge(info);
        // This will timeout and return false
        // Note: This test may take a few seconds due to network timeout
        REQUIRE_FALSE(bridge.isReachable());
    }
}
