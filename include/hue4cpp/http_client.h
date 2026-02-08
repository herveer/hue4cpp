#pragma once

#include "types.h"
#include <string>
#include <map>
#include <chrono>

/**
 * @file http_client.h
 * @brief HTTP client wrapper for Hue API communication
 */

namespace hue4cpp {

/**
 * @brief HTTP response from the Hue bridge
 */
struct HttpResponse {
    int status_code;                          ///< HTTP status code
    std::string body;                         ///< Response body
    std::map<std::string, std::string> headers; ///< Response headers
    std::string error_message;                ///< Error message if request failed
    
    /**
     * @brief Check if the response indicates success
     * @return true if status code is in the 2xx range
     */
    bool isSuccess() const {
        return status_code >= 200 && status_code < 300;
    }
};

/**
 * @brief HTTP client for making requests to the Hue bridge
 * 
 * This class wraps the cpr library to provide a simplified interface
 * for making HTTPS requests to the Hue bridge API.
 */
class HttpClient {
public:
    /**
     * @brief Default constructor
     */
    HttpClient();
    
    /**
     * @brief Destructor
     */
    ~HttpClient() = default;
    
    // Prevent copying
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    // Allow moving
    HttpClient(HttpClient&&) noexcept = default;
    HttpClient& operator=(HttpClient&&) noexcept = default;
    
    /**
     * @brief Set request timeout
     * @param timeout Timeout duration in milliseconds
     */
    void setTimeout(std::chrono::milliseconds timeout);
    
    /**
     * @brief Set whether to verify SSL certificates
     * @param verify true to verify certificates (default), false to skip verification
     * @note For production use, verification should be enabled
     */
    void setVerifySsl(bool verify);
    
    /**
     * @brief Perform HTTP GET request
     * @param url The URL to request
     * @param headers Optional headers to include in the request
     * @return HttpResponse containing the result
     */
    HttpResponse get(const std::string& url,
                     const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Perform HTTP POST request
     * @param url The URL to request
     * @param body The request body (typically JSON)
     * @param headers Optional headers to include in the request
     * @return HttpResponse containing the result
     */
    HttpResponse post(const std::string& url,
                      const std::string& body,
                      const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Perform HTTP PUT request
     * @param url The URL to request
     * @param body The request body (typically JSON)
     * @param headers Optional headers to include in the request
     * @return HttpResponse containing the result
     */
    HttpResponse put(const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers = {});
    
    /**
     * @brief Perform HTTP DELETE request
     * @param url The URL to request
     * @param headers Optional headers to include in the request
     * @return HttpResponse containing the result
     */
    HttpResponse del(const std::string& url,
                     const std::map<std::string, std::string>& headers = {});
    
private:
    std::chrono::milliseconds timeout_;
    bool verify_ssl_;
    
    bool hasContentType(const std::map<std::string, std::string>& headers) const;
};

} // namespace hue4cpp
