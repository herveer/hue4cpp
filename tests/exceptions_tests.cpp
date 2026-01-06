#include <catch2/catch_test_macros.hpp>
#include "hue4cpp/exceptions.h"

using namespace hue4cpp;

TEST_CASE("Exception hierarchy", "[exceptions]") {
    SECTION("HueException construction") {
        HueException ex("Test error");
        REQUIRE(std::string(ex.what()) == "Test error");
        REQUIRE(ex.getErrorCode() == ErrorCode::UnknownError);
    }
    
    SECTION("HueException with error code") {
        HueException ex(ErrorCode::NetworkError, "Network error");
        REQUIRE(std::string(ex.what()) == "Network error");
        REQUIRE(ex.getErrorCode() == ErrorCode::NetworkError);
    }
}

TEST_CASE("NetworkException", "[exceptions]") {
    NetworkException ex("Connection failed");
    REQUIRE(std::string(ex.what()) == "Connection failed");
    REQUIRE(ex.getErrorCode() == ErrorCode::NetworkError);
}

TEST_CASE("AuthenticationException", "[exceptions]") {
    AuthenticationException ex("Invalid credentials");
    REQUIRE(std::string(ex.what()) == "Invalid credentials");
    REQUIRE(ex.getErrorCode() == ErrorCode::AuthenticationFailed);
}

TEST_CASE("ResourceNotFoundException", "[exceptions]") {
    ResourceNotFoundException ex("Light not found");
    REQUIRE(std::string(ex.what()) == "Light not found");
    REQUIRE(ex.getErrorCode() == ErrorCode::ResourceNotFound);
}

TEST_CASE("InvalidParameterException", "[exceptions]") {
    InvalidParameterException ex("Invalid brightness value");
    REQUIRE(std::string(ex.what()) == "Invalid brightness value");
    REQUIRE(ex.getErrorCode() == ErrorCode::InvalidParameter);
}

TEST_CASE("BridgeNotReachableException", "[exceptions]") {
    BridgeNotReachableException ex("Bridge is offline");
    REQUIRE(std::string(ex.what()) == "Bridge is offline");
    REQUIRE(ex.getErrorCode() == ErrorCode::BridgeNotReachable);
}

TEST_CASE("TimeoutException", "[exceptions]") {
    TimeoutException ex("Request timed out");
    REQUIRE(std::string(ex.what()) == "Request timed out");
    REQUIRE(ex.getErrorCode() == ErrorCode::TimeoutError);
}

TEST_CASE("errorCodeToString", "[exceptions]") {
    REQUIRE(errorCodeToString(ErrorCode::Success) == "Success");
    REQUIRE(errorCodeToString(ErrorCode::NetworkError) == "Network Error");
    REQUIRE(errorCodeToString(ErrorCode::AuthenticationRequired) == "Authentication Required");
    REQUIRE(errorCodeToString(ErrorCode::AuthenticationFailed) == "Authentication Failed");
    REQUIRE(errorCodeToString(ErrorCode::InvalidRequest) == "Invalid Request");
    REQUIRE(errorCodeToString(ErrorCode::ResourceNotFound) == "Resource Not Found");
    REQUIRE(errorCodeToString(ErrorCode::InvalidParameter) == "Invalid Parameter");
    REQUIRE(errorCodeToString(ErrorCode::BridgeNotReachable) == "Bridge Not Reachable");
    REQUIRE(errorCodeToString(ErrorCode::TimeoutError) == "Timeout Error");
    REQUIRE(errorCodeToString(ErrorCode::UnknownError) == "Unknown Error");
}

TEST_CASE("Exception polymorphism", "[exceptions]") {
    SECTION("NetworkException is HueException") {
        try {
            throw NetworkException("Network error");
        } catch (const HueException& ex) {
            REQUIRE(ex.getErrorCode() == ErrorCode::NetworkError);
            REQUIRE(true); // Successfully caught as HueException
        }
    }
    
    SECTION("AuthenticationException is HueException") {
        try {
            throw AuthenticationException("Auth error");
        } catch (const HueException& ex) {
            REQUIRE(ex.getErrorCode() == ErrorCode::AuthenticationFailed);
            REQUIRE(true); // Successfully caught as HueException
        }
    }
}
