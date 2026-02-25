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

	void TamperSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("tamper")) {
			auto tamper_obj = delta["tamper"];
			_tampered     = json_utils::getValueOr<bool>(tamper_obj, "tampered",     _tampered);
			_tamper_valid = json_utils::getValueOr<bool>(tamper_obj, "tamper_valid", _tamper_valid);
			NotifyPropertyChanged<&TamperSensor::Tampered>();
			NotifyPropertyChanged<&TamperSensor::TamperValid>();
		}
	}

} // namespace hue4cpp
