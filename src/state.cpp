#include "hue4cpp/state.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/sse_client.h"
#include "hue4cpp/json_utils.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace hue4cpp {

	// StateManager implementation
	StateManager::StateManager()
		: _bridge(nullptr), _running(false) {
	}

	StateManager::StateManager(Bridge* bridge)
		: _bridge(bridge), _running(false) {
	}

	StateManager::~StateManager() {
		stop();
	}

	StateManager::StateManager(StateManager&& other) noexcept
		: _bridge(other._bridge),
		_running(other._running.load()),
		_sse_client(std::move(other._sse_client)),
		_resource_states(std::move(other._resource_states)) {
		other._running = false;
	}

	StateManager& StateManager::operator=(StateManager&& other) noexcept {
		if (this != &other) {
			_bridge = other._bridge;
			_running = other._running.load();
			_sse_client = std::move(other._sse_client);
			_resource_states = std::move(other._resource_states);
			other._running = false;
		}
		return *this;
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

		_sse_client->PropertyChanged += [this](ReactiveLitepp::ObservableObject& obj, ReactiveLitepp::PropertyChangedArgs args) {
			if (args.PropertyName() != nameof::nameof_member<&SSEClient::IsConnected>()) {
				return;
			}

			clearCache();
			if (_sse_client && client->IsConnected) {
				OnResourceEvent.Notify(ResourceEventArgs(EventType::BridgeConnected, ""));
			}
			else {
				OnResourceEvent.Notify(ResourceEventArgs(EventType::BridgeDisconnected, ""));
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
		EventType evtType = EventType::Unknown;
		try {
			auto json = nlohmann::json::parse(state_json);
			if (json.contains("type")) {
				std::string resource_type = json["type"];
				if (resource_type == "light") {
					evtType = EventType::LightStateChanged;
				}

				else if (resource_type == "motion" || resource_type == "temperature" ||
					resource_type == "light_level" || resource_type == "button") {
					evtType = EventType::SensorStateChanged;
				}
			}
		}
		catch (...) {
			// ...
		}
		OnResourceEvent.Notify(ResourceEventArgs(evtType, resource_id, state_json));

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
						if (event_type == "add") {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::LightAdded, resource_id, resource.dump()));
						}
						else if (event_type == "delete") {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::LightRemoved, resource_id));
						}
						else {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::LightStateChanged, resource_id, resource.dump()));
						}
					}
					else if (resource_type == "motion" || resource_type == "temperature" ||
						resource_type == "light_level" || resource_type == "button") {
						if (event_type == "add") {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::SensorAdded, resource_id, resource.dump()));
						}
						else if (event_type == "delete") {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::SensorRemoved, resource_id, resource.dump()));
						}
						else {
							OnResourceEvent.Notify(ResourceEventArgs(EventType::SensorStateChanged, resource_id, resource.dump()));
						}
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
