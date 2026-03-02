#include "hue4cpp/sensors/camera_motion_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	CameraMotionSensor::CameraMotionSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::CameraMotion) {
	}

	SensorType CameraMotionSensor::getType() const {
		return SensorType::CameraMotion;
	}

	void CameraMotionSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("motion") && json["motion"].is_object()) {
			auto motion_obj = json["motion"];
			auto newMotion = json_utils::getValueOr<bool>(motion_obj, "motion", _motion);
			SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotion>(_motion, newMotion);
			auto newValid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
			SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotionValid>(_motion_valid, newValid);
		}
	}

	void CameraMotionSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("motion")) {
				auto motion_obj = delta["motion"];
				auto newMotion = json_utils::getValueOr<bool>(motion_obj, "motion", _motion);
				SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotion>(_motion, newMotion);
				auto newValid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
				SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotionValid>(_motion_valid, newValid);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotion>(_motion, _motion);
			SetPropertyValueAndNotify<&CameraMotionSensor::CameraMotionValid>(_motion_valid, _motion_valid);
		}
	}

} // namespace hue4cpp
