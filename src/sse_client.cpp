#include "hue4cpp/sse_client.h"
#include "hue4cpp/exceptions.h"
#include <cpr/cpr.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <condition_variable>

namespace hue4cpp {

// Constants for SSE client
namespace {
    constexpr int RECONNECT_BACKOFF_MULTIPLIER = 2;
}

// SSEClient implementation
SSEClient::SSEClient(const std::string& url)
    : url_(url)
    , timeout_(std::chrono::minutes(5))
    , verify_ssl_(false)  // Hue bridges use self-signed certs
    , reconnection_enabled_(true)
    , reconnect_initial_delay_(0)
    , reconnect_max_delay_(60)
    , connected_(false)
    , should_run_(false)
{}

SSEClient::~SSEClient() {
    disconnect();
}

SSEClient::SSEClient(SSEClient&& other) noexcept
    : url_(std::move(other.url_))
    , auth_header_name_(std::move(other.auth_header_name_))
    , auth_header_value_(std::move(other.auth_header_value_))
    , timeout_(other.timeout_)
    , verify_ssl_(other.verify_ssl_)
    , reconnection_enabled_(other.reconnection_enabled_)
    , reconnect_initial_delay_(other.reconnect_initial_delay_)
    , reconnect_max_delay_(other.reconnect_max_delay_)
    , connected_(other.connected_.load())
    , should_run_(other.should_run_.load())
    , connection_thread_(std::move(other.connection_thread_))
    , event_callback_(std::move(other.event_callback_))
    , connection_callback_(std::move(other.connection_callback_))
{
    // Reset other's state
    other.connected_ = false;
    other.should_run_ = false;
}

SSEClient& SSEClient::operator=(SSEClient&& other) noexcept {
    if (this != &other) {
        disconnect(); // Clean up our own resources first
        
        url_ = std::move(other.url_);
        auth_header_name_ = std::move(other.auth_header_name_);
        auth_header_value_ = std::move(other.auth_header_value_);
        timeout_ = other.timeout_;
        verify_ssl_ = other.verify_ssl_;
        reconnection_enabled_ = other.reconnection_enabled_;
        reconnect_initial_delay_ = other.reconnect_initial_delay_;
        reconnect_max_delay_ = other.reconnect_max_delay_;
        connected_ = other.connected_.load();
        should_run_ = other.should_run_.load();
        connection_thread_ = std::move(other.connection_thread_);
        event_callback_ = std::move(other.event_callback_);
        connection_callback_ = std::move(other.connection_callback_);
        
        // Reset other's state
        other.connected_ = false;
        other.should_run_ = false;
    }
    return *this;
}

void SSEClient::stopConnection() {
    should_run_ = false;
    if (connection_thread_ && connection_thread_->joinable()) {
        connection_thread_->join();
    }
}

void SSEClient::notifyConnectionChange(bool is_connected) {
    connected_ = is_connected;
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (connection_callback_) {
        connection_callback_(is_connected);
    }
}

void SSEClient::notifyEvent(const SSEEvent& event) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (event_callback_) {
        event_callback_(event);
    }
}

bool SSEClient::parseSSELine(const std::string& line, SSEEvent& current_event) {
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

void SSEClient::connectionLoop() {
    std::chrono::seconds current_retry_delay = reconnect_initial_delay_;
    
    while (should_run_) {
        try {
            // Prepare headers
            cpr::Header headers;
            if (!auth_header_name_.empty() && !auth_header_value_.empty()) {
                headers[auth_header_name_] = auth_header_value_;
            }
            headers["Accept"] = "text/event-stream";
            headers["Cache-Control"] = "no-cache";
            
            // Set up session
            cpr::Session session;
            session.SetUrl(cpr::Url{url_});
            session.SetHeader(headers);
            session.SetTimeout(timeout_);
            session.SetVerifySsl(cpr::VerifySsl{verify_ssl_});
            
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
                
                return should_run_;  // Continue while should_run is true
            };
            
            session.SetWriteCallback(cpr::WriteCallback{write_callback});
            
            // Notify connected
            notifyConnectionChange(true);
            current_retry_delay = reconnect_initial_delay_; // Reset retry delay on successful connection
            
            // Perform the request (this blocks until connection ends)
            auto response = session.Get();
            
            // Connection ended
            notifyConnectionChange(false);
            
            // Check if we should reconnect
            if (!should_run_) {
                break;
            }
            
            if (!reconnection_enabled_) {
                break;
            }
            
        } catch (const std::exception& e) {
            notifyConnectionChange(false);
            
            if (!should_run_ || !reconnection_enabled_) {
                break;
            }
        }
        
        // Wait before reconnecting
        if (should_run_ && reconnection_enabled_) {
            std::this_thread::sleep_for(current_retry_delay);
            
            // Exponential backoff
            if (current_retry_delay == std::chrono::seconds(0)) current_retry_delay = std::chrono::seconds(1);
            current_retry_delay = (std::min)(current_retry_delay * RECONNECT_BACKOFF_MULTIPLIER, reconnect_max_delay_);
        }
    }
}

void SSEClient::setAuthHeader(const std::string& header_name, const std::string& header_value) {
    auth_header_name_ = header_name;
    auth_header_value_ = header_value;
}

void SSEClient::setTimeout(std::chrono::seconds timeout) {
    timeout_ = timeout;
}

void SSEClient::setVerifySsl(bool verify) {
    verify_ssl_ = verify;
}

void SSEClient::setReconnection(bool enabled, std::chrono::seconds initial_delay, std::chrono::seconds max_delay) {
    reconnection_enabled_ = enabled;
    reconnect_initial_delay_ = initial_delay;
    reconnect_max_delay_ = max_delay;
}

void SSEClient::onEvent(SSEEventCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    event_callback_ = callback;
}

void SSEClient::onConnectionChange(SSEConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    connection_callback_ = callback;
}

Result<void> SSEClient::connect() {
    if (should_run_) {
        return Result<void>(ErrorCode::InvalidRequest, "Already connected");
    }
    
    should_run_ = true;
    connection_thread_ = std::make_unique<std::thread>([this]() {
        connectionLoop();
    });
    
    return Result<void>();
}

void SSEClient::disconnect() {
    stopConnection();
    connected_ = false;
}

bool SSEClient::isConnected() const {
    return connected_;
}

} // namespace hue4cpp
