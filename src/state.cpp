#include "hue4cpp/state.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/sse_client.h"
#include "hue4cpp/json_utils.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace hue4cpp {

	// StateManager implementation
	StateManager::StateManager()
		: _bridge(nullptr), _running(false), _next_callback_id(1) {}

	StateManager::StateManager(Bridge* bridge)
		: _bridge(bridge), _running(false), _next_callback_id(1) {}

	StateManager::~StateManager() {
		stop();
	}

	StateManager::StateManager(StateManager&& other) noexcept
		: _bridge(other._bridge),
		  _running(other._running.load()),
		  _sse_client(std::move(other._sse_client)),
		  _resource_states(std::move(other._resource_states)),
		  _callbacks(std::move(other._callbacks)),
		  _next_callback_id(other._next_callback_id) {
		other._running = false;
	}

	StateManager& StateManager::operator=(StateManager&& other) noexcept {
		if (this != &other) {
			_bridge = other._bridge;
			_running = other._running.load();
			_sse_client = std::move(other._sse_client);
			_resource_states = std::move(other._resource_states);
			_callbacks = std::move(other._callbacks);
			_next_callback_id = other._next_callback_id;
			other._running = false;
		}
		return *this;
	}

	void StateManager::notifyCallbacks(const Event& event) {
		std::lock_guard<std::mutex> lock(_callback_mutex);
		for (const auto& [id, callback] : _callbacks) {
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

	void StateManager::mergeResourceState(const std::string& resource_id, const std::string& delta_json) {
		std::lock_guard<std::mutex> lock(_state_mutex);

		auto it = _resource_states.find(resource_id);
		if (it != _resource_states.end()) {
			try {
				auto existing = nlohmann::json::parse(it->second);
				auto delta = nlohmann::json::parse(delta_json);
				existing.merge_patch(delta);
				it->second = existing.dump();
			}
			catch (...) {
				_resource_states[resource_id] = delta_json;
			}
		}
		else {
			_resource_states[resource_id] = delta_json;
		}
	}

	Result<void> StateManager::start() {
		if (_running) {
			return Result<void>(ErrorCode::InvalidRequest, "StateManager already running");
		}

		if (!_bridge) {
			return Result<void>(ErrorCode::InvalidRequest, "No bridge associated with StateManager");
		}

		const auto& bridge_info = _bridge->getInfo();
		if (bridge_info.ip_address.empty()) {
			return Result<void>(ErrorCode::InvalidRequest, "Bridge IP address not set");
		}

		const std::string auth_key = _bridge->getAuthenticationKey();
		if (auth_key.empty()) {
			return Result<void>(ErrorCode::AuthenticationRequired, "Bridge not authenticated");
		}

		const std::string sse_url = "https://" + bridge_info.ip_address + "/eventstream/clip/v2";

		_sse_client = std::make_unique<SSEClient>(sse_url);
		_sse_client->setAuthHeader("hue-application-key", auth_key);
		_sse_client->setVerifySsl(false);
		_sse_client->setReconnection(true);

		_sse_client->OnEvent += [this](const SSEEventArgs& sse_event) {
			updateFromEvent(sse_event.data);
		};

		_sse_client->PropertyChanged += [this]([[maybe_unused]] ReactiveLitepp::ObservableObject& obj, ReactiveLitepp::PropertyChangedArgs args) {
			if(args.PropertyName() != nameof::nameof_member<&SSEClient::IsConnected>()) {
				return;
			}

			clearCache();
			if (_sse_client->IsConnected) {
				Event event(EventType::BridgeConnected, "", "");
				notifyCallbacks(event);
			}
			else {
				Event event(EventType::BridgeDisconnected, "", "");
				notifyCallbacks(event);
			}
		};

		auto result = _sse_client->connect();
		if (!result) {
			_sse_client.reset();
			return result;
		}

		_running = true;
		return Result<void>();
	}

	void StateManager::stop() {
		if (!_running) {
			return;
		}

		if (_sse_client) {
			_sse_client->disconnect();
			_sse_client.reset();
		}

		_running = false;
	}

	bool StateManager::isRunning() const {
		return _running;
	}

	uint64_t StateManager::registerCallback(EventCallback callback) {
		std::lock_guard<std::mutex> lock(_callback_mutex);
		uint64_t id = _next_callback_id++;

		if (_next_callback_id == 0) {
			_next_callback_id = 1;
		}

		_callbacks[id] = callback;
		return id;
	}

	void StateManager::unregisterCallback(uint64_t callback_id) {
		std::lock_guard<std::mutex> lock(_callback_mutex);
		_callbacks.erase(callback_id);
	}

	std::string StateManager::getResourceState(const std::string& resource_id) const {
		std::lock_guard<std::mutex> lock(_state_mutex);
		if (!_sse_client || !(bool)_sse_client->IsConnected) {
			return "";
		}

		auto it = _resource_states.find(resource_id);
		if (it != _resource_states.end()) {
			return it->second;
		}
		return "";
	}

	void StateManager::setResourceState(const std::string& resource_id, const std::string& state_json) {
		std::lock_guard<std::mutex> lock(_state_mutex);
		if (!isRunning()) {
			start();
		}
		_resource_states[resource_id] = state_json;
	}

	void StateManager::updateFromEvent(const std::string& event_json) {
		if (event_json.empty()) {
			return;
		}

		try {
			auto json = nlohmann::json::parse(event_json);

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

				for (const auto& resource : data_array) {
					if (!resource.contains("id") || !resource.contains("type")) {
						continue;
					}

					std::string resource_id = resource["id"];
					std::string resource_type = resource["type"];

					if (event_type == "delete") {
						std::lock_guard<std::mutex> lock(_state_mutex);
						_resource_states.erase(resource_id);
					}
					else {
						mergeResourceState(resource_id, resource.dump());
					}

					if (resource_type == "light") {
						EventType evt_type = EventType::LightStateChanged;
						if (event_type == "add") {
							evt_type = EventType::LightAdded;
						}
						else if (event_type == "delete") {
							evt_type = EventType::LightRemoved;
						}

						std::string current_state = getResourceState(resource_id);
						Event event(evt_type, resource_id, resource.dump());
						notifyCallbacks(event);
					}
					else if (resource_type == "motion" || resource_type == "temperature" ||
						resource_type == "light_level" || resource_type == "button") {
						EventType evt_type = EventType::SensorStateChanged;
						if (event_type == "add") {
							evt_type = EventType::SensorAdded;
						}
						else if (event_type == "delete") {
							evt_type = EventType::SensorRemoved;
						}

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
		_bridge = bridge;
	}

	void StateManager::clearCache() {
		std::lock_guard<std::mutex> lock(_state_mutex);
		_resource_states.clear();
	}

} // namespace hue4cpp
