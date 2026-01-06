#pragma once

#include "types.h"
#include <functional>
#include <string>

/**
 * @file state.h
 * @brief State management and event handling
 */

namespace hue4cpp {

// Forward declarations
class Light;
class Bridge;

/**
 * @brief Event types for state changes
 */
enum class EventType {
    LightStateChanged,
    LightAdded,
    LightRemoved,
    BridgeConnected,
    BridgeDisconnected,
    Unknown
};

/**
 * @brief Event data structure
 */
struct Event {
    EventType type;
    std::string resource_id;
    std::string data; // JSON data
    
    Event(EventType t = EventType::Unknown,
          const std::string& id = "",
          const std::string& json_data = "")
        : type(t), resource_id(id), data(json_data) {}
};

/**
 * @brief Callback function for state change events
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Manages state synchronization via Server-Sent Events
 * 
 * The StateManager maintains an up-to-date view of the bridge state
 * by listening to SSE events from the bridge.
 */
class StateManager {
public:
    /**
     * @brief Default constructor
     */
    StateManager();
    
    /**
     * @brief Construct with parent bridge
     * @param bridge Pointer to parent bridge
     */
    explicit StateManager(Bridge* bridge);
    
    /**
     * @brief Destructor
     */
    ~StateManager();
    
    // Prevent copying, allow moving
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    StateManager(StateManager&&) noexcept;
    StateManager& operator=(StateManager&&) noexcept;
    
    /**
     * @brief Start listening for SSE events
     * @return Result indicating success or failure
     */
    Result<void> start();
    
    /**
     * @brief Stop listening for SSE events
     */
    void stop();
    
    /**
     * @brief Check if state manager is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Register a callback for state change events
     * @param callback Function to call when events occur
     * @return Callback ID for later removal
     */
    uint64_t registerCallback(EventCallback callback);
    
    /**
     * @brief Unregister a previously registered callback
     * @param callback_id ID returned from registerCallback
     */
    void unregisterCallback(uint64_t callback_id);
    
    /**
     * @brief Get the current state of a light
     * @param light_id Light unique identifier
     * @return JSON string representing light state, or empty if not found
     */
    std::string getLightState(const std::string& light_id) const;
    
    /**
     * @brief Update internal state from JSON event
     * @param event_json JSON event data from SSE
     */
    void updateFromEvent(const std::string& event_json);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace hue4cpp
