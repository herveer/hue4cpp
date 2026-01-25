#include "hue4cpp/sse_client.h"
#include "hue4cpp/exceptions.h"
#include <cpr/cpr.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <condition_variable>

namespace hue4cpp {

// SSEClient::Impl definition
class SSEClient::Impl {
public:
    std::string url;
    std::string auth_header_name;
    std::string auth_header_value;
    std::chrono::seconds timeout;
    bool verify_ssl;
    
    // Reconnection settings
    bool reconnection_enabled;
    std::chrono::seconds reconnect_initial_delay;
    std::chrono::seconds reconnect_max_delay;
    
    // Connection state
    std::atomic<bool> connected;
    std::atomic<bool> should_run;
    std::unique_ptr<std::thread> connection_thread;
    
    // Callbacks
    SSEEventCallback event_callback;
    SSEConnectionCallback connection_callback;
    std::mutex callback_mutex;
    
    Impl(const std::string& endpoint_url)
        : url(endpoint_url)
        , timeout(30)
        , verify_ssl(false)  // Hue bridges use self-signed certs
        , reconnection_enabled(true)
        , reconnect_initial_delay(1)
        , reconnect_max_delay(60)
        , connected(false)
        , should_run(false)
    {}
    
    ~Impl() {
        stopConnection();
    }
    
    void stopConnection() {
        should_run = false;
        if (connection_thread && connection_thread->joinable()) {
            connection_thread->join();
        }
    }
    
    void notifyConnectionChange(bool is_connected) {
        connected = is_connected;
        std::lock_guard<std::mutex> lock(callback_mutex);
        if (connection_callback) {
            connection_callback(is_connected);
        }
    }
    
    void notifyEvent(const SSEEvent& event) {
        std::lock_guard<std::mutex> lock(callback_mutex);
        if (event_callback) {
            event_callback(event);
        }
    }
    
    /**
     * @brief Parse SSE stream data
     * @param line Line from SSE stream
     * @param current_event Current event being built
     * @return true if event is complete, false otherwise
     */
    bool parseSSELine(const std::string& line, SSEEvent& current_event) {
        if (line.empty()) {
            // Empty line signals end of event
            return !current_event.data.empty();
        }
        
        if (line[0] == ':') {
            // Comment line, ignore
            return false;
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            // Field with no value
            return false;
        }
        
        std::string field = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        
        // Skip leading space in value
        if (!value.empty() && value[0] == ' ') {
            value = value.substr(1);
        }
        
        if (field == "event") {
            current_event.event_type = value;
        } else if (field == "data") {
            if (!current_event.data.empty()) {
                current_event.data += "\n";
            }
            current_event.data += value;
        } else if (field == "id") {
            current_event.id = value;
        }
        // retry field is ignored for now
        
        return false;
    }
    
    void connectionLoop() {
        std::chrono::seconds current_retry_delay = reconnect_initial_delay;
        
        while (should_run) {
            try {
                // Prepare headers
                cpr::Header headers;
                if (!auth_header_name.empty() && !auth_header_value.empty()) {
                    headers[auth_header_name] = auth_header_value;
                }
                headers["Accept"] = "text/event-stream";
                headers["Cache-Control"] = "no-cache";
                
                // Set up session
                cpr::Session session;
                session.SetUrl(cpr::Url{url});
                session.SetHeader(headers);
                session.SetTimeout(timeout);
                session.SetVerifySsl(cpr::VerifySsl{verify_ssl});
                
                // Create a custom write callback to process SSE stream
                std::string buffer;
                SSEEvent current_event;
                
                std::function<bool(std::string_view, intptr_t)> write_callback = 
                    [&](std::string_view data, intptr_t) -> bool {
                    buffer.append(data.data(), data.size());
                    
                    // Process complete lines
                    size_t newline_pos;
                    while ((newline_pos = buffer.find('\n')) != std::string::npos) {
                        std::string line = buffer.substr(0, newline_pos);
                        buffer = buffer.substr(newline_pos + 1);
                        
                        // Remove \r if present
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }
                        
                        // Parse line
                        if (parseSSELine(line, current_event)) {
                            // Event complete, notify callback
                            notifyEvent(current_event);
                            current_event = SSEEvent();
                        }
                    }
                    
                    return should_run;  // Continue while should_run is true
                };
                
                session.SetWriteCallback(cpr::WriteCallback{write_callback});
                
                // Notify connected
                notifyConnectionChange(true);
                current_retry_delay = reconnect_initial_delay; // Reset retry delay on successful connection
                
                // Perform the request (this blocks until connection ends)
                auto response = session.Get();
                
                // Connection ended
                notifyConnectionChange(false);
                
                // Check if we should reconnect
                if (!should_run) {
                    break;
                }
                
                if (!reconnection_enabled) {
                    break;
                }
                
            } catch (const std::exception& e) {
                notifyConnectionChange(false);
                
                if (!should_run || !reconnection_enabled) {
                    break;
                }
            }
            
            // Wait before reconnecting
            if (should_run && reconnection_enabled) {
                std::this_thread::sleep_for(current_retry_delay);
                
                // Exponential backoff
                current_retry_delay = std::min(current_retry_delay * 2, reconnect_max_delay);
            }
        }
    }
};

// SSEClient implementation
SSEClient::SSEClient(const std::string& url)
    : pImpl(std::make_unique<Impl>(url))
{}

SSEClient::~SSEClient() {
    disconnect();
}

SSEClient::SSEClient(SSEClient&& other) noexcept : pImpl(std::move(other.pImpl)) {
    // pImpl is moved, other.pImpl is now nullptr
}

SSEClient& SSEClient::operator=(SSEClient&& other) noexcept {
    if (this != &other) {
        disconnect(); // Clean up our own resources first
        pImpl = std::move(other.pImpl);
    }
    return *this;
}

void SSEClient::setAuthHeader(const std::string& header_name, const std::string& header_value) {
    pImpl->auth_header_name = header_name;
    pImpl->auth_header_value = header_value;
}

void SSEClient::setTimeout(std::chrono::seconds timeout) {
    pImpl->timeout = timeout;
}

void SSEClient::setVerifySsl(bool verify) {
    pImpl->verify_ssl = verify;
}

void SSEClient::setReconnection(bool enabled, std::chrono::seconds initial_delay, std::chrono::seconds max_delay) {
    pImpl->reconnection_enabled = enabled;
    pImpl->reconnect_initial_delay = initial_delay;
    pImpl->reconnect_max_delay = max_delay;
}

void SSEClient::onEvent(SSEEventCallback callback) {
    std::lock_guard<std::mutex> lock(pImpl->callback_mutex);
    pImpl->event_callback = callback;
}

void SSEClient::onConnectionChange(SSEConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(pImpl->callback_mutex);
    pImpl->connection_callback = callback;
}

Result<void> SSEClient::connect() {
    if (pImpl->should_run) {
        return Result<void>(ErrorCode::InvalidRequest, "Already connected");
    }
    
    pImpl->should_run = true;
    pImpl->connection_thread = std::make_unique<std::thread>([this]() {
        pImpl->connectionLoop();
    });
    
    return Result<void>();
}

void SSEClient::disconnect() {
    if (pImpl) {
        pImpl->stopConnection();
        pImpl->connected = false;
    }
}

bool SSEClient::isConnected() const {
    return pImpl && pImpl->connected;
}

} // namespace hue4cpp
