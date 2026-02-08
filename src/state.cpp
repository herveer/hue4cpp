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

	// StateManager implementation
	StateManager::StateManager()
		: bridge_(nullptr), running_(false), next_callback_id_(1) {}

	StateManager::StateManager(Bridge* bridge)
		: bridge_(bridge), running_(false), next_callback_id_(1) {}

	StateManager::~StateManager() {
		stop();
	}

	StateManager::StateManager(StateManager&& other) noexcept
		: bridge_(other.bridge_)
		, running_(other.running_.load())
		, sse_client_(std::move(other.sse_client_))
		, resource_states_(std::move(other.resource_states_))
		, callbacks_(std::move(other.callbacks_))
		, next_callback_id_(other.next_callback_id_)
	{
		other.bridge_ = nullptr;
		other.running_ = false;
		other.next_callback_id_ = 1;
	}

	StateManager& StateManager::operator=(StateManager&& other) noexcept {
		if (this != &other) {
			stop(); // Clean up our own resources first
			
			bridge_ = other.bridge_;
			running_ = other.running_.load();
			sse_client_ = std::move(other.sse_client_);
			resource_states_ = std::move(other.resource_states_);
			callbacks_ = std::move(other.callbacks_);
			next_callback_id_ = other.next_callback_id_;
			
			other.bridge_ = nullptr;
			other.running_ = false;
			other.next_callback_id_ = 1;
		}
		return *this;
	}

	void StateManager::notifyCallbacks(const Event& event) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		for (const auto& [id, callback] : callbacks_) {
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

	void StateManager::mergeResourceState(const std::string& resource_id, const nlohmann::json& delta) {
		std::lock_guard<std::mutex> lock(state_mutex_);

		auto it = resource_states_.find(resource_id);
		if (it != resource_states_.end()) {
			// Existing state - merge delta
			try {
				auto existing = nlohmann::json::parse(it->second);
				existing.merge_patch(delta);
				it->second = existing.dump();
			}
			catch (...) {
				// If parse fails, replace with delta
				resource_states_[resource_id] = delta.dump();
			}
		}
		else {
			// No existing state - store delta as initial state
			resource_states_[resource_id] = delta.dump();
		}
	}

	Result<void> StateManager::start() {
		if (running_) {
			return Result<void>(ErrorCode::InvalidRequest, "StateManager already running");
		}

		if (!bridge_) {
			return Result<void>(ErrorCode::InvalidRequest, "No bridge associated with StateManager");
		}

		// Get bridge info for SSE endpoint
		const auto& bridge_info = bridge_->getInfo();
		if (bridge_info.ip_address.empty()) {
			return Result<void>(ErrorCode::InvalidRequest, "Bridge IP address not set");
		}

		const std::string auth_key = bridge_->getAuthenticationKey();
		if (auth_key.empty()) {
			return Result<void>(ErrorCode::AuthenticationRequired, "Bridge not authenticated");
		}

		// Create SSE endpoint URL
		const std::string sse_url = "https://" + bridge_info.ip_address + "/eventstream/clip/v2";

		// Create and configure SSE client
		sse_client_ = std::make_unique<SSEClient>(sse_url);
		sse_client_->setAuthHeader("hue-application-key", auth_key);
		sse_client_->setVerifySsl(false); // Hue bridges use self-signed certificates
		sse_client_->setReconnection(true);

		// Set up event callback
		sse_client_->onEvent([this](const SSEEvent& sse_event) {
			// Process SSE event and update state
			updateFromEvent(sse_event.data);
			});

		// Set up connection state callback
		sse_client_->onConnectionChange([this](bool connected) {
			// Clear cache on disconnect/reconnect
			clearCache();
			if (connected) {
				Event event(EventType::BridgeConnected, "", "");
				notifyCallbacks(event);
			}
			else {
				Event event(EventType::BridgeDisconnected, "", "");
				notifyCallbacks(event);
			}
			});

		// Connect to SSE stream
		auto result = sse_client_->connect();
		if (!result) {
			sse_client_.reset();
			return result;
		}

		running_ = true;
		return Result<void>();
	}

	void StateManager::stop() {
		if (!running_) {
			return;
		}

		if (sse_client_) {
			sse_client_->disconnect();
			sse_client_.reset();
		}

		running_ = false;
	}

	bool StateManager::isRunning() const {
		return running_;
	}

	uint64_t StateManager::registerCallback(EventCallback callback) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		uint64_t id = next_callback_id_++;

		// Handle overflow (unlikely but defensive)
		if (next_callback_id_ == 0) {
			next_callback_id_ = 1;
		}

		callbacks_[id] = callback;
		return id;
	}

	void StateManager::unregisterCallback(uint64_t callback_id) {
		std::lock_guard<std::mutex> lock(callback_mutex_);
		callbacks_.erase(callback_id);
	}

	std::string StateManager::getResourceState(const std::string& resource_id) const {
		std::lock_guard<std::mutex> lock(state_mutex_);
		if (!sse_client_->isConnected()) {
			// If sse is not connected the state is not reliable
			return "";
		}

		auto it = resource_states_.find(resource_id);
		if (it != resource_states_.end()) {
			return it->second;
		}
		return "";
	}

	void StateManager::setResourceState(const std::string& resource_id, const std::string& state_json) {
		std::lock_guard<std::mutex> lock(state_mutex_);
		if (!isRunning()) {
			// try to start the SSE client if not running
			start();
		}
		resource_states_[resource_id] = state_json;
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

					// Update internal state cache with delta merge
					if (event_type == "delete") {
						// Remove from cache on delete
						std::lock_guard<std::mutex> lock(state_mutex_);
						resource_states_.erase(resource_id);
					}
					else {
						// Merge delta into existing state for add/update events
						mergeResourceState(resource_id, resource);
					}

					// Handle light state updates
					if (resource_type == "light") {
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
						notifyCallbacks(event);
					}
					// Handle sensor state updates (motion, temperature, light_level, button)
					else if (resource_type == "motion" || resource_type == "temperature" ||
						resource_type == "light_level" || resource_type == "button") {
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
						notifyCallbacks(event);
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
		bridge_ = bridge;
	}

	void StateManager::clearCache() {
		std::lock_guard<std::mutex> lock(state_mutex_);
		resource_states_.clear();
	}

} // namespace hue4cpp
