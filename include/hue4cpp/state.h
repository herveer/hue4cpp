#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>
#include <map>
#include <atomic>
#include <mutex>

/**
 * @file state.h
 * @brief State management and event handling
 */

namespace hue4cpp {

	// Forward declarations
	class Light;
	class Sensor;
	class Bridge;
	class SSEClient;

	/**
	 * @brief Event types for state changes
	 */
	enum class EventType {
		LightStateChanged,
		LightAdded,
		LightRemoved,
		SensorStateChanged,
		SensorAdded,
		SensorRemoved,
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
			: type(t), resource_id(id), data(json_data) {
		}
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
		 * @brief Get the current state of any resource (generic)
		 * @param resource_id Resource unique identifier
		 * @return JSON string representing resource state, or empty if not found
		 */
		std::string getResourceState(const std::string& resource_id) const;

		/**
		 * @brief Update resource state (replaces entire state, used for API calls)
		 * @param resource_id Resource unique identifier
		 * @param state_json JSON string representing complete resource state
		 */
		void setResourceState(const std::string& resource_id, const std::string& state_json);

		/**
		 * @brief Update internal state from JSON event
		 * @param event_json JSON event data from SSE
		 */
		void updateFromEvent(const std::string& event_json);

		/**
		 * @brief Set the parent bridge
		 * @param bridge Pointer to parent bridge
		 */
		void setBridge(Bridge* bridge);

		/**
		 * @brief Clear all cached resource states
		 */
		void clearCache();

	private:
		Bridge* bridge_;
		std::atomic<bool> running_;
		std::unique_ptr<SSEClient> sse_client_;

		// Thread-safe state management - generic for all resource types
		mutable std::mutex state_mutex_;
		std::map<std::string, std::string> resource_states_; // resource_id -> JSON state

		// Thread-safe callback management
		std::mutex callback_mutex_;
		std::map<uint64_t, EventCallback> callbacks_;
		uint64_t next_callback_id_;

		void notifyCallbacks(const Event& event);
		void mergeResourceState(const std::string& resource_id, const nlohmann::json& delta);
	};

} // namespace hue4cpp
