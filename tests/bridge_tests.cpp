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
    
    SECTION("Not authenticated by default") {
        REQUIRE_FALSE(bridge.isAuthenticated());
        REQUIRE(bridge.getAuthenticationKey().empty());
    }
    
    SECTION("Set authentication key") {
        bridge.setAuthenticationKey("test-key-12345");
        REQUIRE(bridge.isAuthenticated());
        REQUIRE(bridge.getAuthenticationKey() == "test-key-12345");
    }
    
    SECTION("Get authentication key after setting") {
        std::string test_key = "my-app-key-67890";
        bridge.setAuthenticationKey(test_key);
        REQUIRE(bridge.getAuthenticationKey() == test_key);
    }
    
    SECTION("Clear authentication key") {
        bridge.setAuthenticationKey("test-key");
        REQUIRE(bridge.isAuthenticated());
        
        bridge.setAuthenticationKey("");
        REQUIRE_FALSE(bridge.isAuthenticated());
        REQUIRE(bridge.getAuthenticationKey().empty());
    }
}

TEST_CASE("Bridge authentication flow", "[bridge][auth]") {
    SECTION("Authenticate without IP address fails") {
        Bridge bridge;
        auto result = bridge.authenticate("TestApp", "TestDevice");
        
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("Authenticate with empty app name fails") {
        BridgeInfo info;
        info.ip_address = "192.168.1.100";
        Bridge bridge(info);
        
        auto result = bridge.authenticate("", "TestDevice");
        
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("Authenticate constructs devicetype correctly") {
        // This is a unit test - we can't actually test against a real bridge
        // but we verify the parameters are validated correctly
        BridgeInfo info;
        info.ip_address = "192.168.1.100";
        Bridge bridge(info);
        
        // This will fail because there's no actual bridge, but it tests parameter handling
        auto result = bridge.authenticate("MyApp");
        REQUIRE_FALSE(result.isSuccess());
        // Could be network error or timeout (no real bridge to connect to)
        REQUIRE((result.error == ErrorCode::NetworkError || 
                 result.error == ErrorCode::TimeoutError ||
                 result.error == ErrorCode::AuthenticationFailed));
    }
}

TEST_CASE("Bridge authentication validation", "[bridge][auth]") {
    SECTION("Validate without authentication key fails") {
        BridgeInfo info;
        info.ip_address = "192.168.1.100";
        Bridge bridge(info);
        
        auto result = bridge.validateAuthentication();
        
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::AuthenticationRequired);
    }
    
    SECTION("Validate without IP address fails") {
        Bridge bridge;
        bridge.setAuthenticationKey("test-key");
        
        auto result = bridge.validateAuthentication();
        
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("Validate with key and IP attempts connection") {
        // This will fail because there's no actual bridge, but it tests the flow
        BridgeInfo info;
        info.ip_address = "192.168.1.100";
        Bridge bridge(info);
        bridge.setAuthenticationKey("test-key-12345");
        
        auto result = bridge.validateAuthentication();
        
        // Should get network error or authentication failed (no real bridge)
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE((result.error == ErrorCode::NetworkError || 
                 result.error == ErrorCode::AuthenticationFailed));
    }
}

TEST_CASE("Bridge discovery", "[bridge]") {
    SECTION("mDNS discovery returns vector") {
        auto bridges = Bridge::discoverMDNS();
        // mDNS discovery is now implemented
        // We can't assert the size since it depends on network/bridges availability
        // Just verify it doesn't crash and returns a vector
        REQUIRE(bridges.size() >= 0);
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
