#include "hue4cpp/sensors/sensor_base.h"
#include "hue4cpp/sensors/motion_sensor.h"
#include "hue4cpp/sensors/temperature_sensor.h"
#include "hue4cpp/sensors/light_level_sensor.h"
#include "hue4cpp/sensors/button_sensor.h"
#include "hue4cpp/sensors/camera_motion_sensor.h"
#include "hue4cpp/sensors/bell_button_sensor.h"
#include "hue4cpp/sensors/relative_rotary_sensor.h"
#include "hue4cpp/sensors/geolocation_sensor.h"
#include "hue4cpp/sensors/tamper_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/state.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/exceptions.h"
#include <iostream>

namespace hue4cpp {

	namespace {
		// Helper function to determine sensor type from API type string
		SensorType parseSensorType(const std::string& type_str) {
			if (type_str == "motion") return SensorType::Motion;
			if (type_str == "temperature") return SensorType::Temperature;
			if (type_str == "light_level") return SensorType::LightLevel;
			if (type_str == "button") return SensorType::Button;
			if (type_str == "camera_motion") return SensorType::CameraMotion;
			if (type_str == "bell_button") return SensorType::BellButton;
			if (type_str == "relative_rotary") return SensorType::RelativeRotary;
			if (type_str == "geolocation") return SensorType::Geolocation;
			if (type_str == "tamper") return SensorType::Tamper;
			return SensorType::Unknown;
		}
	}

	// Base Sensor implementation
	Sensor::Sensor(const std::string& id, Bridge* bridge, SensorType type)
		: _id(id), _bridge(bridge), _type(type) {
			subscribeToBridgeEvents();
	}

	std::string Sensor::getResourceTypeString() const {
		switch (_type) {
		case SensorType::Motion:
			return "motion";
		case SensorType::Temperature:
			return "temperature";
		case SensorType::LightLevel:
			return "light_level";
		case SensorType::Button:
			return "button";
		case SensorType::CameraMotion:
			return "camera_motion";
		case SensorType::BellButton:
			return "bell_button";
		case SensorType::RelativeRotary:
			return "relative_rotary";
		case SensorType::Geolocation:
			return "geolocation";
		case SensorType::Tamper:
			return "tamper";
		default:
			return "";
		}
	}

	Bridge* Sensor::getBridge() const {
		return _bridge;
	}

	bool Sensor::isEnabled() const {
		return _enabled;
	}

	Result<void> Sensor::refresh() {
		if (!_bridge) {
			return Result<void>(ErrorCode::InvalidParameter, "No bridge associated with sensor");
		}

		std::string state = _bridge->getSensorState(_id, getResourceTypeString(), true);
		
		if (state.empty()) {
			return Result<void>(ErrorCode::NetworkError, "Failed to refresh sensor state");
		}

		return Result<void>();
	}

	void Sensor::initFromJson(const nlohmann::json& json) {
		try {
			if (json.contains("id")) {
				auto idVal = json_utils::getValueOr<std::string>(json, "id", _id);
				SetPropertyValueAndNotify<&Sensor::Id>(_id, idVal);
			}

			if (json.contains("metadata") && json["metadata"].is_object()) {
				auto nameVal = json_utils::getValueOr<std::string>(json["metadata"], "name", "");
				SetPropertyValueAndNotify<&Sensor::Name>(_name, nameVal);
			}

			if (json.contains("enabled")) {
				auto val = json_utils::getValueOr<bool>(json, "enabled", _enabled);
				SetPropertyValueAndNotify<&Sensor::Enabled>(_enabled, val);
			}

			if (_bridge && !_id.empty()) {
				_bridge->getStateManager().setResourceState(_id, json.dump());
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing sensor JSON: " << e.what() << std::endl;
		}
	}

	void Sensor::subscribeToBridgeEvents() {
		if (!_bridge) {
			return;
		}

		_bridgeEventSubscription = _bridge->getStateManager().OnResourceEvent.SubscribeScoped(
			[this](const ResourceEventArgs& e) {
				onResourceEvent(e);
			});
	}

	void Sensor::onResourceEvent(const ResourceEventArgs& e) {
		// Only react to sensor state changes that concern this specific sensor.
		if (!e.isSensorEvent() || e.type != EventType::SensorStateChanged || e.resource_id != _id) {
			return;
		}

		// Fire the generic per-sensor event so any external subscriber is notified.
		OnStateChanged.Notify(e);

		// Let the derived class fire its typed property notifications.
		try {
			auto delta = nlohmann::json::parse(e.state_json);

			if (delta.contains("enabled")) {
				auto val = json_utils::getValueOr<bool>(delta, "enabled", _enabled);
				SetPropertyValueAndNotify<&Sensor::Enabled>(_enabled, val);
      }
      
			// Metadata rename pushed from the bridge.
			if (delta.contains("metadata") && delta["metadata"].is_object()) {
				const auto& meta = delta["metadata"];
				if (meta.contains("name") && meta["name"].is_string()) {
					auto newName = meta["name"].get<std::string>();
					SetPropertyValueAndNotify<&Sensor::Name>(_name, newName);
				}
			}

			notifyStateProperties(delta);
		}
		catch (...) {
			// Malformed delta — derived class will handle its own fallback.
		}
	}

	void Sensor::sendUpdate(const nlohmann::json& state_update) {
		if (!_bridge || !_bridge->isAuthenticated()) {
			throw BridgeNotReachableException("Bridge not authenticated");
		}

		const auto& bridge_info = _bridge->getInfo();
		if (bridge_info.ip_address.empty() || _id.empty()) {
			throw InvalidParameterException("Bridge IP or sensor ID not set");
		}

		try {
			HttpClient client;
			client.setVerifySsl(false);
			client.setTimeout(std::chrono::milliseconds(5000));

			std::string url = "https://" + bridge_info.ip_address +
				"/clip/v2/resource/" + getResourceTypeString() + "/" + _id;

			std::map<std::string, std::string> headers;
			headers["hue-application-key"] = _bridge->getAuthenticationKey();

			std::string body = json_utils::toString(state_update);
			auto response = client.put(url, body, headers);

			if (!response.isSuccess()) {
				if (response.status_code == 401 || response.status_code == 403) {
					throw AuthenticationException("Authentication failed");
				}
				throw NetworkException("HTTP request failed: " + response.error_message);
			}

			auto json_response = json_utils::parse(response.body);
			if (json_response.contains("errors") && json_response["errors"].is_array()) {
				const auto& errors = json_response["errors"];
				if (!errors.empty()) {
					throw InvalidParameterException(
						json_utils::getValueOr<std::string>(errors[0], "description", "Unknown error"));
				}
			}
		}
		catch (const HueException&) { throw; }
		catch (const std::exception& e) {
			throw BridgeNotReachableException(std::string("Error: ") + e.what());
		}
	}

	void Sensor::setName(const std::string& name) {
		if (name.empty()) {
			throw InvalidParameterException("Sensor name must not be empty");
		}

		nlohmann::json update;
		update["metadata"] = { {"name", name} };
		sendUpdate(update);

		// Update the backing field immediately so reads are consistent
		// while waiting for the SSE confirmation round-trip.
		_name = name;
	}

	// Factory function to create appropriate sensor type from JSON
	std::unique_ptr<Sensor> createSensorFromJson(const nlohmann::json& json, Bridge* bridge) {
		if (!json.contains("type") || !json.contains("id")) {
			return nullptr;
		}

		std::string type_str = json_utils::getValueOr<std::string>(json, "type", "");
		std::string id = json_utils::getValueOr<std::string>(json, "id", "");
		
		SensorType type = parseSensorType(type_str);
		
		std::unique_ptr<Sensor> sensor;
		
		switch (type) {
		case SensorType::Motion:
			sensor = std::make_unique<MotionSensor>(id, bridge);
			break;
		case SensorType::Temperature:
			sensor = std::make_unique<TemperatureSensor>(id, bridge);
			break;
		case SensorType::LightLevel:
			sensor = std::make_unique<LightLevelSensor>(id, bridge);
			break;
		case SensorType::Button:
			sensor = std::make_unique<ButtonSensor>(id, bridge);
			break;
		case SensorType::CameraMotion:
			sensor = std::make_unique<CameraMotionSensor>(id, bridge);
			break;
		case SensorType::BellButton:
			sensor = std::make_unique<BellButtonSensor>(id, bridge);
			break;
		case SensorType::RelativeRotary:
			sensor = std::make_unique<RelativeRotarySensor>(id, bridge);
			break;
		case SensorType::Geolocation:
			sensor = std::make_unique<GeolocationSensor>(id, bridge);
			break;
		case SensorType::Tamper:
			sensor = std::make_unique<TamperSensor>(id, bridge);
			break;
		default:
			return nullptr;
		}
		
		if (sensor) {
			sensor->initFromJson(json);
		}
		
		return sensor;
	}

} // namespace hue4cpp
