#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/bridge.h>

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
    SECTION("Discover returns empty vector (not implemented)") {
        auto bridges = Bridge::discover();
        // Should return empty since not implemented yet
        REQUIRE(bridges.empty());
    }
}

TEST_CASE("Bridge state manager", "[bridge]") {
    Bridge bridge;
    
    SECTION("State manager is available") {
        auto& state_manager = bridge.getStateManager();
        REQUIRE_FALSE(state_manager.isRunning());
    }
}
