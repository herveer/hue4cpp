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

		// Thread-safe state management
		std::mutex state_mutex;
		std::map<std::string, std::string> light_states; // light_id -> JSON state

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

		std::string auth_key = pImpl->bridge->getAuthenticationKey();
		if (auth_key.empty()) {
			return Result<void>(ErrorCode::AuthenticationRequired, "Bridge not authenticated");
		}

		// Create SSE endpoint URL
		std::string sse_url = "https://" + bridge_info.ip_address + "/eventstream/clip/v2";

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

	std::string StateManager::getLightState(const std::string& light_id) const {
		std::lock_guard<std::mutex> lock(pImpl->state_mutex);
		auto it = pImpl->light_states.find(light_id);
		if (it != pImpl->light_states.end()) {
			return it->second;
		}
		return "";
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
						// Update internal state cache
						{
							std::lock_guard<std::mutex> lock(pImpl->state_mutex);
							if (event_type == "delete") {
								// Remove from cache on delete
								pImpl->light_states.erase(resource_id);
							}
							else {
								// Add or update cache for add/update events
								pImpl->light_states[resource_id] = resource.dump();
							}
						}

						// Determine event type
						EventType evt_type = EventType::LightStateChanged;
						if (event_type == "add") {
							evt_type = EventType::LightAdded;
						}
						else if (event_type == "delete") {
							evt_type = EventType::LightRemoved;
						}

						// Notify callbacks
						Event event(evt_type, resource_id, resource.dump());
						pImpl->notifyCallbacks(event);
					}
					// Can add more resource types here (rooms, zones, scenes, etc.)
				}
			}
		}
		catch (const std::exception&) {
			// Ignore JSON parsing errors
		}
	}

	void StateManager::setBridge(Bridge* bridge) {
		pImpl->bridge = bridge;
	}


} // namespace hue4cpp
