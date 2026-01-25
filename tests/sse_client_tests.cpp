#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/sse_client.h>
#include <thread>
#include <chrono>

using namespace hue4cpp;

TEST_CASE("SSEClient construction", "[sse_client]") {
    SECTION("Constructor with URL") {
        SSEClient client("https://example.com/events");
        REQUIRE_FALSE(client.isConnected());
    }
}

TEST_CASE("SSEClient configuration", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Set auth header") {
        client.setAuthHeader("Authorization", "Bearer token123");
        REQUIRE_FALSE(client.isConnected());
    }
    
    SECTION("Set timeout") {
        client.setTimeout(std::chrono::seconds(10));
        REQUIRE_FALSE(client.isConnected());
    }
    
    SECTION("Set SSL verification") {
        client.setVerifySsl(false);
        REQUIRE_FALSE(client.isConnected());
    }
    
    SECTION("Set reconnection") {
        client.setReconnection(true, std::chrono::seconds(1), std::chrono::seconds(30));
        REQUIRE_FALSE(client.isConnected());
    }
}

TEST_CASE("SSEClient callbacks", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Register event callback") {
        bool callback_called = false;
        client.onEvent([&callback_called](const SSEEvent& event) {
            callback_called = true;
        });
        REQUIRE_FALSE(callback_called);
    }
    
    SECTION("Register connection callback") {
        bool callback_called = false;
        client.onConnectionChange([&callback_called](bool connected) {
            callback_called = true;
        });
        REQUIRE_FALSE(callback_called);
    }
}

TEST_CASE("SSEClient disconnect without connect", "[sse_client]") {
    SSEClient client("https://example.com/events");
    
    SECTION("Can disconnect without connecting") {
        client.disconnect();
        REQUIRE_FALSE(client.isConnected());
    }
}

TEST_CASE("SSEEvent structure", "[sse_client]") {
    SECTION("Default construction") {
        SSEEvent event;
        REQUIRE(event.event_type.empty());
        REQUIRE(event.data.empty());
        REQUIRE(event.id.empty());
    }
    
    SECTION("Construction with parameters") {
        SSEEvent event("update", "{\"key\":\"value\"}", "event-123");
        REQUIRE(event.event_type == "update");
        REQUIRE(event.data == "{\"key\":\"value\"}");
        REQUIRE(event.id == "event-123");
    }
}

TEST_CASE("SSEClient move semantics", "[sse_client]") {
    SECTION("Move constructor") {
        SSEClient client1("https://example.com/events");
        SSEClient client2(std::move(client1));
        REQUIRE_FALSE(client2.isConnected());
    }
    
    SECTION("Move assignment") {
        SSEClient client1("https://example.com/events");
        SSEClient client2("https://other.com/events");
        client2 = std::move(client1);
        REQUIRE_FALSE(client2.isConnected());
    }
}
