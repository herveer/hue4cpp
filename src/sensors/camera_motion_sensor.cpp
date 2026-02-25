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

	void CameraMotionSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("motion")) {
			auto motion_obj = delta["motion"];
			_motion       = json_utils::getValueOr<bool>(motion_obj, "motion",       _motion);
			_motion_valid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
			NotifyPropertyChanged<&CameraMotionSensor::CameraMotion>();
			NotifyPropertyChanged<&CameraMotionSensor::CameraMotionValid>();
		}
	}

} // namespace hue4cpp
