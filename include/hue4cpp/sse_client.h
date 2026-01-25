#pragma once

#include "types.h"
#include <functional>
#include <string>
#include <memory>
#include <chrono>

/**
 * @file sse_client.h
 * @brief Server-Sent Events (SSE) client for real-time updates
 */

namespace hue4cpp {

/**
 * @brief SSE event data structure
 */
struct SSEEvent {
    std::string event_type; ///< Event type (e.g., "message", "update")
    std::string data;       ///< Event data payload
    std::string id;         ///< Event ID (optional)
    
    SSEEvent() = default;
    SSEEvent(const std::string& type, const std::string& payload, const std::string& event_id = "")
        : event_type(type), data(payload), id(event_id) {}
};

/**
 * @brief Callback function for SSE events
 */
using SSEEventCallback = std::function<void(const SSEEvent&)>;

/**
 * @brief Callback function for connection state changes
 */
using SSEConnectionCallback = std::function<void(bool connected)>;

/**
 * @brief SSE client for listening to Server-Sent Events
 * 
 * This class handles connection to an SSE endpoint, parsing events,
 * automatic reconnection, and event routing.
 */
class SSEClient {
public:
    /**
     * @brief Constructor
     * @param url The SSE endpoint URL
     */
    explicit SSEClient(const std::string& url);
    
    /**
     * @brief Destructor
     */
    ~SSEClient();
    
    // Prevent copying, allow moving
    SSEClient(const SSEClient&) = delete;
    SSEClient& operator=(const SSEClient&) = delete;
    SSEClient(SSEClient&&) noexcept;
    SSEClient& operator=(SSEClient&&) noexcept;
    
    /**
     * @brief Set authentication header
     * @param header_name Name of the header (e.g., "hue-application-key")
     * @param header_value Value of the header
     */
    void setAuthHeader(const std::string& header_name, const std::string& header_value);
    
    /**
     * @brief Set connection timeout
     * @param timeout Timeout duration in seconds
     */
    void setTimeout(std::chrono::seconds timeout);
    
    /**
     * @brief Set whether to verify SSL certificates
     * @param verify true to verify certificates, false to skip
     */
    void setVerifySsl(bool verify);
    
    /**
     * @brief Set automatic reconnection parameters
     * @param enabled Enable automatic reconnection
     * @param initial_delay Initial delay before reconnect attempt
     * @param max_delay Maximum delay between reconnect attempts
     */
    void setReconnection(bool enabled, 
                        std::chrono::seconds initial_delay = std::chrono::seconds(1),
                        std::chrono::seconds max_delay = std::chrono::seconds(60));
    
    /**
     * @brief Register callback for SSE events
     * @param callback Function to call when events are received
     */
    void onEvent(SSEEventCallback callback);
    
    /**
     * @brief Register callback for connection state changes
     * @param callback Function to call when connection state changes
     */
    void onConnectionChange(SSEConnectionCallback callback);
    
    /**
     * @brief Start listening for SSE events
     * @return Result indicating success or failure
     */
    Result<void> connect();
    
    /**
     * @brief Stop listening for SSE events
     */
    void disconnect();
    
    /**
     * @brief Check if currently connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace hue4cpp
