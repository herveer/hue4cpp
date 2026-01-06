#pragma once

#include "types.h"
#include <stdexcept>
#include <string>

/**
 * @file exceptions.h
 * @brief Exception classes for hue4cpp
 */

namespace hue4cpp {

/**
 * @brief Base exception class for all hue4cpp exceptions
 */
class HueException : public std::runtime_error {
public:
    explicit HueException(const std::string& message)
        : std::runtime_error(message), error_code_(ErrorCode::UnknownError) {}
    
    HueException(ErrorCode code, const std::string& message)
        : std::runtime_error(message), error_code_(code) {}
    
    /**
     * @brief Get the error code associated with this exception
     * @return ErrorCode value
     */
    ErrorCode getErrorCode() const noexcept { return error_code_; }

private:
    ErrorCode error_code_;
};

/**
 * @brief Exception thrown when network operations fail
 */
class NetworkException : public HueException {
public:
    explicit NetworkException(const std::string& message)
        : HueException(ErrorCode::NetworkError, message) {}
};

/**
 * @brief Exception thrown when authentication fails
 */
class AuthenticationException : public HueException {
public:
    explicit AuthenticationException(const std::string& message)
        : HueException(ErrorCode::AuthenticationFailed, message) {}
};

/**
 * @brief Exception thrown when a requested resource is not found
 */
class ResourceNotFoundException : public HueException {
public:
    explicit ResourceNotFoundException(const std::string& message)
        : HueException(ErrorCode::ResourceNotFound, message) {}
};

/**
 * @brief Exception thrown when invalid parameters are provided
 */
class InvalidParameterException : public HueException {
public:
    explicit InvalidParameterException(const std::string& message)
        : HueException(ErrorCode::InvalidParameter, message) {}
};

/**
 * @brief Exception thrown when the bridge is not reachable
 */
class BridgeNotReachableException : public HueException {
public:
    explicit BridgeNotReachableException(const std::string& message)
        : HueException(ErrorCode::BridgeNotReachable, message) {}
};

/**
 * @brief Exception thrown when an operation times out
 */
class TimeoutException : public HueException {
public:
    explicit TimeoutException(const std::string& message)
        : HueException(ErrorCode::TimeoutError, message) {}
};

/**
 * @brief Convert ErrorCode to a human-readable string
 * @param code The error code to convert
 * @return String representation of the error code
 */
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return "Success";
        case ErrorCode::NetworkError:
            return "Network Error";
        case ErrorCode::AuthenticationRequired:
            return "Authentication Required";
        case ErrorCode::AuthenticationFailed:
            return "Authentication Failed";
        case ErrorCode::InvalidRequest:
            return "Invalid Request";
        case ErrorCode::ResourceNotFound:
            return "Resource Not Found";
        case ErrorCode::InvalidParameter:
            return "Invalid Parameter";
        case ErrorCode::BridgeNotReachable:
            return "Bridge Not Reachable";
        case ErrorCode::TimeoutError:
            return "Timeout Error";
        case ErrorCode::UnknownError:
            return "Unknown Error";
        default:
            return "Unknown";
    }
}

} // namespace hue4cpp
