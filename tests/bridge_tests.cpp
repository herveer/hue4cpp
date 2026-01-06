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
    SECTION("Discover returns vector (may be empty if no bridges found)") {
        auto bridges = Bridge::discover();
        // Should return a vector (empty or with bridges)
        // We can't guarantee bridges will be found in test environment
        REQUIRE(true); // Test passes if no exception thrown
    }
    
    SECTION("DiscoverRemote returns vector") {
        auto bridges = Bridge::discoverRemote();
        // Remote discovery should complete without throwing
        // Result may be empty if no bridges are on the same network
        REQUIRE(true); // Test passes if no exception thrown
    }
    
    SECTION("DiscoverMDNS returns vector") {
        auto bridges = Bridge::discoverMDNS();
        // mDNS discovery is now implemented
        // Result may be empty if no bridges are available on the network
        REQUIRE(true); // Test passes if no exception thrown
    }
}

TEST_CASE("Bridge reachability", "[bridge]") {
    SECTION("Bridge with no IP is not reachable") {
        Bridge bridge;
        REQUIRE_FALSE(bridge.isReachable());
    }
    
    SECTION("Bridge with invalid IP is not reachable") {
        BridgeInfo info;
        info.ip_address = "999.999.999.999";
        info.id = "test-id";
        
        Bridge bridge(info);
        REQUIRE_FALSE(bridge.isReachable());
    }
}

TEST_CASE("Bridge state manager", "[bridge]") {
    Bridge bridge;
    
    SECTION("State manager is available") {
        auto& state_manager = bridge.getStateManager();
        REQUIRE_FALSE(state_manager.isRunning());
    }
}
