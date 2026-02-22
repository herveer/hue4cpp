#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/sse_client.h>
#include <thread>
#include <chrono>

using namespace hue4cpp;

TEST_CASE("SSEClient construction", "[sse_client]") {
    SECTION("Constructor with URL") {
        SSEClient client("https://example.com/events");
        REQUIRE_FALSE((bool)client.IsConnected);
    }
}

TEST_CASE("SSEClient configuration", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Set auth header") {
        client.setAuthHeader("Authorization", "Bearer token123");
        REQUIRE_FALSE((bool)client.IsConnected);
    }
    
    SECTION("Set timeout") {
        client.setTimeout(std::chrono::seconds(10));
        REQUIRE_FALSE((bool)client.IsConnected);
    }
    
    SECTION("Set SSL verification") {
        client.setVerifySsl(false);
        REQUIRE_FALSE((bool)client.IsConnected);
    }
    
    SECTION("Set reconnection") {
        client.setReconnection(true, std::chrono::seconds(1), std::chrono::seconds(30));
        REQUIRE_FALSE((bool)client.IsConnected);
    }
}

TEST_CASE("SSEClient events", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Subscribe to OnEvent") {
        bool callback_called = false;
        client.OnEvent += [&callback_called](const SSEEventArgs&) {
            callback_called = true;
        };
        REQUIRE_FALSE(callback_called);
    }
    
    SECTION("Subscribe to ConnectionChanged") {
        bool callback_called = false;
        client.PropertyChanged += [&callback_called](ReactiveLitepp::ObservableObject& obj, ReactiveLitepp::PropertyChangeArgs args) {
            callback_called = true;
        };
        REQUIRE_FALSE(callback_called);
    }
}

TEST_CASE("SSEClient disconnect without connect", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Can disconnect without connecting") {
        client.disconnect();
        REQUIRE_FALSE((bool)client.IsConnected);
    }
}

TEST_CASE("SSEEvent structure", "[sse_client]") {
    SECTION("Default construction") {
        SSEEventArgs event;
        REQUIRE(event.event_type.empty());
        REQUIRE(event.data.empty());
        REQUIRE(event.id.empty());
    }
    
    SECTION("Construction with parameters") {
        SSEEventArgs event("update", "{\"key\":\"value\"}", "event-123");
        REQUIRE(event.event_type == "update");
        REQUIRE(event.data == "{\"key\":\"value\"}");
        REQUIRE(event.id == "event-123");
    }
}

TEST_CASE("SSEClient IsConnected property", "[sse_client]") {
    SECTION("IsConnected is false by default") {
        SSEClient client("https://example.com/events");
        REQUIRE_FALSE((bool)client.IsConnected);
    }

    SECTION("IsConnected is false after disconnect") {
        SSEClient client("https://example.com/events");
        client.disconnect();
        REQUIRE_FALSE((bool)client.IsConnected);
    }
}
