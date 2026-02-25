#include "hue4cpp/sensors/light_level_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	LightLevelSensor::LightLevelSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::LightLevel) {
	}

	SensorType LightLevelSensor::getType() const {
		return SensorType::LightLevel;
	}

	void LightLevelSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("light")) {
			auto light_obj     = delta["light"];
			_light_level       = json_utils::getValueOr<uint32_t>(light_obj, "light_level",       _light_level);
			_light_level_valid = json_utils::getValueOr<bool>(light_obj,     "light_level_valid",  _light_level_valid);
			NotifyPropertyChanged<&LightLevelSensor::LightLevel>();
			NotifyPropertyChanged<&LightLevelSensor::LightLevelValid>();
		}
	}

} // namespace hue4cpp
