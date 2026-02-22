#pragma once

#include "types.h"
#include <ReactiveLitepp/Event.h>
#include <ReactiveLitepp/Property.h>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <memory>

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
 * @brief SSE client for listening to Server-Sent Events
 * 
 * This class handles connection to an SSE endpoint, parsing events,
 * automatic reconnection, and event routing.
 *
 * @note The IsConnected property holds a reference to this object (via lambda
 *       capture). Callers must not use IsConnected after the SSEClient has been
 *       destroyed.
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
    
    // Non-copyable, non-movable (properties capture this)
    SSEClient(const SSEClient&) = delete;
    SSEClient& operator=(const SSEClient&) = delete;
    SSEClient(SSEClient&&) = delete;
    SSEClient& operator=(SSEClient&&) = delete;
    
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
                         std::chrono::seconds initial_delay = std::chrono::seconds(0),
                         std::chrono::seconds max_delay = std::chrono::seconds(60));
    
    /**
     * @brief Event fired when an SSE event is received
     */
    ReactiveLitepp::Event<const SSEEvent&> OnEvent;

    /**
     * @brief Event fired when the connection state changes
     *        Argument is true when connected, false when disconnected
     */
    ReactiveLitepp::Event<bool> ConnectionChanged;

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
     * @brief Read-only property indicating whether the client is currently connected
     */
    ReactiveLitepp::ReadonlyProperty<bool> IsConnected{
        [this]() { return _connected.load(); }
    };

private:
    std::string _url;
    std::string _auth_header_name;
    std::string _auth_header_value;
    std::chrono::seconds _timeout;
    bool _verify_ssl;

    // Reconnection settings
    bool _reconnection_enabled;
    std::chrono::seconds _reconnect_initial_delay;
    std::chrono::seconds _reconnect_max_delay;

    // Connection state
    std::atomic<bool> _connected;
    std::atomic<bool> _should_run;
    std::unique_ptr<std::thread> _connection_thread;

    bool parseSSELine(const std::string& line, SSEEvent& current_event);
    void connectionLoop();
};

} // namespace hue4cpp
