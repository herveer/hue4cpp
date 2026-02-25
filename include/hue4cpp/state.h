#pragma once

#include "types.h"
#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <memory>
#include <ReactiveLitepp/Event.h>

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
	 * @brief Event args for Bridge::LightStateChanged
	 *
	 * Carries all information needed by Light objects (and other
	 * subscribers) to react to a state change reported by the bridge.
	 */
	struct LightStateChangedArgs {
		std::string light_id;   ///< Unique identifier of the affected light
		std::string state_json; ///< Updated state as a JSON string

		LightStateChangedArgs() = default;
		LightStateChangedArgs(const std::string& id, const std::string& json)
			: light_id(id), state_json(json) {
		}

		/**
		 * @brief Construct from a generic Event
		 * @param event Source event (must have EventType::LightStateChanged)
		 */
		explicit LightStateChangedArgs(const Event& event)
			: light_id(event.resource_id), state_json(event.data) {
		}
	};

	/**
	 * @brief Unified event args for StateManager::OnResourceEvent
	 *
	 * Carries all information about any resource change, addition, or removal
	 * fired by the StateManager. Inspect @c type to determine what happened
	 * and which fields are meaningful.
	 *
	 * | type                    | resource_id | state_json       |
	 * |-------------------------|-------------|------------------|
	 * | LightStateChanged       | light id    | updated JSON     |
	 * | LightAdded              | light id    | initial JSON     |
	 * | LightRemoved            | light id    | empty            |
	 * | SensorStateChanged      | sensor id   | updated JSON     |
	 * | SensorAdded             | sensor id   | initial JSON     |
	 * | SensorRemoved           | sensor id   | empty            |
	 * | BridgeConnected         | empty       | empty            |
	 * | BridgeDisconnected      | empty       | empty            |
	 */
	struct ResourceEventArgs {
		EventType   type;         ///< What happened
		std::string resource_id;  ///< Affected resource id (empty for bridge events)
		std::string state_json;   ///< Resource state JSON (empty when not applicable)

		ResourceEventArgs()
			: type(EventType::Unknown) {
		}

		ResourceEventArgs(EventType t,
		                  const std::string& id,
		                  const std::string& json = "")
			: type(t), resource_id(id), state_json(json) {
		}

		// Convenience helpers ---------------------------------------------------

		/** @brief Returns true when this event concerns a light resource */
		bool isLightEvent() const {
			return type == EventType::LightStateChanged
				|| type == EventType::LightAdded
				|| type == EventType::LightRemoved;
		}

		/** @brief Returns true when this event concerns a sensor resource */
		bool isSensorEvent() const {
			return type == EventType::SensorStateChanged
				|| type == EventType::SensorAdded
				|| type == EventType::SensorRemoved;
		}

		/** @brief Returns true when this event concerns bridge connectivity */
		bool isBridgeEvent() const {
			return type == EventType::BridgeConnected
				|| type == EventType::BridgeDisconnected;
		}
	};

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

		// ----------------------------------------------------------------
		// ReactiveLitepp event — subscribe with += or SubscribeScoped()
		// ----------------------------------------------------------------

		/**
		 * @brief Single unified event fired for every resource change.
		 *
		 * Covers lights (added / changed / removed), sensors (added / changed /
		 * removed) and bridge connectivity changes.  Use ResourceEventArgs::type
		 * or the helper predicates (isLightEvent, isSensorEvent, isBridgeEvent)
		 * to filter the events you care about.
		 *
		 * @code
		 * auto sub = state_manager.OnResourceEvent.SubscribeScoped(
		 *     [](const ResourceEventArgs& e) {
		 *         if (e.isLightEvent())   { ... }
		 *         if (e.isSensorEvent())  { ... }
		 *         if (e.isBridgeEvent())  { ... }
		 *     });
		 * @endcode
		 */
		ReactiveLitepp::Event<const ResourceEventArgs&> OnResourceEvent;

		// ----------------------------------------------------------------

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
		Bridge* _bridge;
		std::atomic<bool> _running;
		std::unique_ptr<SSEClient> _sse_client;

		mutable std::mutex _state_mutex;
		std::map<std::string, std::string> _resource_states;

		void mergeResourceState(const std::string& resource_id, const std::string& delta_json);
	};

} // namespace hue4cpp
