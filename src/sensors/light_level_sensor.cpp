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

	void LightLevelSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("light") && json["light"].is_object()) {
			auto light_obj = json["light"];
			auto newLevel = json_utils::getValueOr<uint32_t>(light_obj, "light_level", _light_level);
			SetPropertyValueAndNotify<&LightLevelSensor::LightLevel>(_light_level, newLevel);
			auto newValid = json_utils::getValueOr<bool>(light_obj, "light_level_valid", _light_level_valid);
			SetPropertyValueAndNotify<&LightLevelSensor::LightLevelValid>(_light_level_valid, newValid);
		}
	}

	void LightLevelSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("light")) {
				auto light_obj = delta["light"];
				auto newLevel = json_utils::getValueOr<uint32_t>(light_obj, "light_level", _light_level);
				SetPropertyValueAndNotify<&LightLevelSensor::LightLevel>(_light_level, newLevel);
				auto newValid = json_utils::getValueOr<bool>(light_obj, "light_level_valid", _light_level_valid);
				SetPropertyValueAndNotify<&LightLevelSensor::LightLevelValid>(_light_level_valid, newValid);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&LightLevelSensor::LightLevel>(_light_level, _light_level);
			SetPropertyValueAndNotify<&LightLevelSensor::LightLevelValid>(_light_level_valid, _light_level_valid);
		}
	}

} // namespace hue4cpp
