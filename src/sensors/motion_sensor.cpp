#include "hue4cpp/sensors/motion_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	MotionSensor::MotionSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::Motion) {
	}

	SensorType MotionSensor::getType() const {
		return SensorType::Motion;
	}

	void MotionSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("motion") && json["motion"].is_object()) {
			auto motion_obj = json["motion"];
			auto newMotion = json_utils::getValueOr<bool>(motion_obj, "motion", _motion);
			SetPropertyValueAndNotify<&MotionSensor::Motion>(_motion, newMotion);
			auto newValid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
			SetPropertyValueAndNotify<&MotionSensor::MotionValid>(_motion_valid, newValid);
		}
	}

	void MotionSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("motion")) {
				auto motion_obj = delta["motion"];
				auto newMotion = json_utils::getValueOr<bool>(motion_obj, "motion", _motion);
				SetPropertyValueAndNotify<&MotionSensor::Motion>(_motion, newMotion);
				auto newValid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
				SetPropertyValueAndNotify<&MotionSensor::MotionValid>(_motion_valid, newValid);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&MotionSensor::Motion>(_motion, _motion);
			SetPropertyValueAndNotify<&MotionSensor::MotionValid>(_motion_valid, _motion_valid);
		}
	}

} // namespace hue4cpp
