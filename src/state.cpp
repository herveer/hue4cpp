#include "hue4cpp/state.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/sse_client.h"
#include "hue4cpp/json_utils.h"
#include <nlohmann/json.hpp>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>

namespace hue4cpp {

	// StateManager::Impl definition
	class StateManager::Impl {
	public:
		Bridge* bridge;
		std::atomic<bool> running;
		std::unique_ptr<SSEClient> sse_client;

		// Thread-safe state management - generic for all resource types
		std::mutex state_mutex;
		std::map<std::string, std::string> resource_states; // resource_id -> JSON state

		// Thread-safe callback management
		std::mutex callback_mutex;
		std::map<uint64_t, EventCallback> callbacks;
		uint64_t next_callback_id;

		Impl() : bridge(nullptr), running(false), next_callback_id(1) {}
		explicit Impl(Bridge* parent_bridge)
			: bridge(parent_bridge), running(false), next_callback_id(1) {
		}

		void notifyCallbacks(const Event& event) {
			std::lock_guard<std::mutex> lock(callback_mutex);
			for (const auto& [id, callback] : callbacks) {
				if (callback) {
					try {
						callback(event);
					}
					catch (...) {
						// Ignore exceptions from user callbacks
					}
				}
			}
		}

		// Merge JSON delta into existing state
		void mergeResourceState(const std::string& resource_id, const nlohmann::json& delta) {
			std::lock_guard<std::mutex> lock(state_mutex);

			auto it = resource_states.find(resource_id);
			if (it != resource_states.end()) {
				// Existing state - merge delta
				try {
					auto existing = nlohmann::json::parse(it->second);
					existing.merge_patch(delta);
					it->second = existing.dump();
				}
				catch (...) {
					// If parse fails, replace with delta
					resource_states[resource_id] = delta.dump();
				}
			}
			else {
				// No existing state - store delta as initial state
				resource_states[resource_id] = delta.dump();
			}
		}
	};

	// StateManager implementation
	StateManager::StateManager() : pImpl(std::make_unique<Impl>()) {}

	StateManager::StateManager(Bridge* bridge) : pImpl(std::make_unique<Impl>(bridge)) {}

	StateManager::~StateManager() {
		stop();
	}

	StateManager::StateManager(StateManager&& other) noexcept : pImpl(std::move(other.pImpl)) {}

	StateManager& StateManager::operator=(StateManager&&) noexcept = default;

	Result<void> StateManager::start() {
		if (pImpl->running) {
			return Result<void>(ErrorCode::InvalidRequest, "StateManager already running");
		}

		if (!pImpl->bridge) {
			return Result<void>(ErrorCode::InvalidRequest, "No bridge associated with StateManager");
		}

		// Get bridge info for SSE endpoint
		const auto& bridge_info = pImpl->bridge->getInfo();
		if (bridge_info.ip_address.empty()) {
			return Result<void>(ErrorCode::InvalidRequest, "Bridge IP address not set");
		}

		const std::string auth_key = pImpl->bridge->getAuthenticationKey();
		if (auth_key.empty()) {
			return Result<void>(ErrorCode::AuthenticationRequired, "Bridge not authenticated");
		}

		// Create SSE endpoint URL
		const std::string sse_url = "https://" + bridge_info.ip_address + "/eventstream/clip/v2";

		// Create and configure SSE client
		pImpl->sse_client = std::make_unique<SSEClient>(sse_url);
		pImpl->sse_client->setAuthHeader("hue-application-key", auth_key);
		pImpl->sse_client->setVerifySsl(false); // Hue bridges use self-signed certificates
		pImpl->sse_client->setReconnection(true);

		// Set up event callback
		pImpl->sse_client->onEvent([this](const SSEEvent& sse_event) {
			// Process SSE event and update state
			updateFromEvent(sse_event.data);
			});

		// Set up connection state callback
		pImpl->sse_client->onConnectionChange([this](bool connected) {
			// Clear cache on disconnect/reconnect
			clearCache();
			if (connected) {
				Event event(EventType::BridgeConnected, "", "");
				pImpl->notifyCallbacks(event);
			}
			else {
				Event event(EventType::BridgeDisconnected, "", "");
				pImpl->notifyCallbacks(event);
			}
			});

		// Connect to SSE stream
		auto result = pImpl->sse_client->connect();
		if (!result) {
			pImpl->sse_client.reset();
			return result;
		}

		pImpl->running = true;
		return Result<void>();
	}

	void StateManager::stop() {
		if (!pImpl->running) {
			return;
		}

		if (pImpl->sse_client) {
			pImpl->sse_client->disconnect();
			pImpl->sse_client.reset();
		}

		pImpl->running = false;
	}

	bool StateManager::isRunning() const {
		return pImpl->running;
	}

	uint64_t StateManager::registerCallback(EventCallback callback) {
		std::lock_guard<std::mutex> lock(pImpl->callback_mutex);
		uint64_t id = pImpl->next_callback_id++;

		// Handle overflow (unlikely but defensive)
		if (pImpl->next_callback_id == 0) {
			pImpl->next_callback_id = 1;
		}

		pImpl->callbacks[id] = callback;
		return id;
	}

	void StateManager::unregisterCallback(uint64_t callback_id) {
		std::lock_guard<std::mutex> lock(pImpl->callback_mutex);
		pImpl->callbacks.erase(callback_id);
	}

	std::string StateManager::getResourceState(const std::string& resource_id) const {
		std::lock_guard<std::mutex> lock(pImpl->state_mutex);
		if(!pImpl->sse_client->isConnected()) {
			// If sse is not connected the state is not reliable
			return "";
		}

		auto it = pImpl->resource_states.find(resource_id);
		if (it != pImpl->resource_states.end()) {
			return it->second;
		}
		return "";
	}

	void StateManager::setResourceState(const std::string& resource_id, const std::string& state_json) {
		std::lock_guard<std::mutex> lock(pImpl->state_mutex);
		if (!isRunning()) {
			// try to start the SSE client if not running
			start();
		}
		pImpl->resource_states[resource_id] = state_json;
	}

	void StateManager::updateFromEvent(const std::string& event_json) {
		if (event_json.empty()) {
			return;
		}

		try {
			// Parse the event JSON
			auto json = nlohmann::json::parse(event_json);

			// SSE events from Hue API V2 have this structure:
			// [
			//   {
			//     "creationtime": "2023-...",
			//     "data": [ { "id": "...", "type": "light", ... } ],
			//     "id": "...",
			//     "type": "update"
			//   }
			// ]

			if (!json.is_array()) {
				return;
			}

			for (const auto& event_item : json) {
				if (!event_item.contains("data") || !event_item.is_object()) {
					continue;
				}

				std::string event_type = event_item.value("type", "");
				const auto& data_array = event_item["data"];

				if (!data_array.is_array()) {
					continue;
				}

				// Process each resource in the event
				for (const auto& resource : data_array) {
					if (!resource.contains("id") || !resource.contains("type")) {
						continue;
					}

					std::string resource_id = resource["id"];
					std::string resource_type = resource["type"];

					// Handle light state updates
					if (resource_type == "light") {
						// Update internal state cache with delta merge
						if (event_type == "delete") {
							// Remove from cache on delete
							std::lock_guard<std::mutex> lock(pImpl->state_mutex);
							pImpl->resource_states.erase(resource_id);
						}
						else {
							// Merge delta into existing state for add/update events
							pImpl->mergeResourceState(resource_id, resource);
						}

						// Determine event type
						EventType evt_type = EventType::LightStateChanged;
						if (event_type == "add") {
							evt_type = EventType::LightAdded;
						}
						else if (event_type == "delete") {
							evt_type = EventType::LightRemoved;
						}

						// Notify callbacks with current state
						std::string current_state = getResourceState(resource_id);
						Event event(evt_type, resource_id, resource.dump());
						pImpl->notifyCallbacks(event);
					}
					// Handle sensor state updates (motion, temperature, light_level, button)
					else if (resource_type == "motion" || resource_type == "temperature" ||
						resource_type == "light_level" || resource_type == "button") {
						// Update internal state cache with delta merge
						if (event_type == "delete") {
							// Remove from cache on delete
							std::lock_guard<std::mutex> lock(pImpl->state_mutex);
							pImpl->resource_states.erase(resource_id);
						}
						else {
							// Merge delta into existing state for add/update events
							pImpl->mergeResourceState(resource_id, resource);
						}

						// Determine event type
						EventType evt_type = EventType::SensorStateChanged;
						if (event_type == "add") {
							evt_type = EventType::SensorAdded;
						}
						else if (event_type == "delete") {
							evt_type = EventType::SensorRemoved;
						}

						// Notify callbacks with event data
						Event event(evt_type, resource_id, resource.dump());
						pImpl->notifyCallbacks(event);
					}
					// TODO: Add more resource types here (rooms, zones, scenes, etc.)
				}
			}
		}
		catch (const std::exception& e) {
			std::cout << "StateManager::updateFromEvent: Failed to parse event JSON " << e.what() << std::endl;
		}
	}

	void StateManager::setBridge(Bridge* bridge) {
		pImpl->bridge = bridge;
	}

	void StateManager::clearCache() {
		std::lock_guard<std::mutex> lock(pImpl->state_mutex);
		pImpl->resource_states.clear();
	}

} // namespace hue4cpp
