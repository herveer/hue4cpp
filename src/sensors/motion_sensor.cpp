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

	void MotionSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("motion")) {
			auto motion_obj = delta["motion"];
			_motion       = json_utils::getValueOr<bool>(motion_obj, "motion",       _motion);
			_motion_valid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", _motion_valid);
			NotifyPropertyChanged<&MotionSensor::Motion>();
			NotifyPropertyChanged<&MotionSensor::MotionValid>();
		}
	}

} // namespace hue4cpp
