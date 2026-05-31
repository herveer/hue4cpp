#include "hue4cpp/sensors/tamper_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	TamperSensor::TamperSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::Tamper) {
	}

	SensorType TamperSensor::getType() const {
		return SensorType::Tamper;
	}

	void TamperSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("tamper") && json["tamper"].is_object()) {
			auto tamper_obj = json["tamper"];
			auto newTampered = json_utils::getValueOr<bool>(tamper_obj, "tampered", _tampered);
			SetPropertyValueAndNotify<&TamperSensor::Tampered>(_tampered, newTampered);
			auto newValid = json_utils::getValueOr<bool>(tamper_obj, "tamper_valid", _tamper_valid);
			SetPropertyValueAndNotify<&TamperSensor::TamperValid>(_tamper_valid, newValid);
		}
	}

	void TamperSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("tamper")) {
				auto tamper_obj = delta["tamper"];
				auto newTampered = json_utils::getValueOr<bool>(tamper_obj, "tampered", _tampered);
				SetPropertyValueAndNotify<&TamperSensor::Tampered>(_tampered, newTampered);
				auto newValid = json_utils::getValueOr<bool>(tamper_obj, "tamper_valid", _tamper_valid);
				SetPropertyValueAndNotify<&TamperSensor::TamperValid>(_tamper_valid, newValid);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&TamperSensor::Tampered>(_tampered, _tampered);
			SetPropertyValueAndNotify<&TamperSensor::TamperValid>(_tamper_valid, _tamper_valid);
		}
	}

} // namespace hue4cpp
