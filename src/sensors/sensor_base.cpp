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
	}

	std::string Sensor::getId() const {
		return _id;
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
		// Extract enabled state from cached JSON
		std::function extractEnabled = [](const std::string& state_json) -> std::optional<bool> {
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("enabled") && state["enabled"].is_boolean()) {
						return state["enabled"].template get<bool>();
					}
				}
				catch (...) {
					// Fall through to refresh cache or return default
				}
			}
			return std::nullopt;
		};

		if (!_bridge) {
			return false;
		}

		std::string state_json = _bridge->getSensorState(_id, getResourceTypeString(), false);
		auto enabled_opt = extractEnabled(state_json);
		if (enabled_opt.has_value()) {
			return enabled_opt.value();
		}

		state_json = _bridge->getSensorState(_id, getResourceTypeString(), true);
		enabled_opt = extractEnabled(state_json);
		return enabled_opt.value_or(false);
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
				_id = json_utils::getValueOr<std::string>(json, "id", _id);
			}

			if (_bridge && !_id.empty()) {
				_bridge->getStateManager().setResourceState(_id, json.dump());
			}
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing sensor JSON: " << e.what() << std::endl;
		}
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
