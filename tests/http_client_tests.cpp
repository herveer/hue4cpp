#include <catch2/catch_test_macros.hpp>
#include "hue4cpp/http_client.h"

using namespace hue4cpp;

TEST_CASE("HttpClient construction", "[http_client]") {
    SECTION("Default constructor") {
        HttpClient client;
        // Client should be constructible
        REQUIRE(true);
    }
}

TEST_CASE("HttpClient timeout configuration", "[http_client]") {
    HttpClient client;
    
    SECTION("Set timeout") {
        // Should not throw
        REQUIRE_NOTHROW(client.setTimeout(std::chrono::milliseconds(5000)));
    }
}

TEST_CASE("HttpClient SSL verification configuration", "[http_client]") {
    HttpClient client;
    
    SECTION("Enable SSL verification") {
        REQUIRE_NOTHROW(client.setVerifySsl(true));
    }
    
    SECTION("Disable SSL verification") {
        REQUIRE_NOTHROW(client.setVerifySsl(false));
    }
}

TEST_CASE("HttpResponse success check", "[http_client]") {
    HttpResponse response;
    
    SECTION("Success status codes") {
        response.status_code = 200;
        REQUIRE(response.isSuccess());
        
        response.status_code = 201;
        REQUIRE(response.isSuccess());
        
        response.status_code = 299;
        REQUIRE(response.isSuccess());
    }
    
    SECTION("Failure status codes") {
        response.status_code = 199;
        REQUIRE_FALSE(response.isSuccess());
        
        response.status_code = 300;
        REQUIRE_FALSE(response.isSuccess());
        
        response.status_code = 400;
        REQUIRE_FALSE(response.isSuccess());
        
        response.status_code = 404;
        REQUIRE_FALSE(response.isSuccess());
        
        response.status_code = 500;
        REQUIRE_FALSE(response.isSuccess());
    }
}

TEST_CASE("HttpClient move semantics", "[http_client]") {
    SECTION("Move constructor") {
        HttpClient client1;
        client1.setTimeout(std::chrono::milliseconds(3000));
        
        HttpClient client2(std::move(client1));
        // Should not throw
        REQUIRE(true);
    }
    
    SECTION("Move assignment") {
        HttpClient client1;
        client1.setTimeout(std::chrono::milliseconds(3000));
        
        HttpClient client2;
        client2 = std::move(client1);
        // Should not throw
        REQUIRE(true);
    }
}

// Note: Integration tests with actual HTTP requests would require a test server
// or mock implementation, which is beyond the scope of basic unit tests.
// These tests verify the API contract and basic functionality.
