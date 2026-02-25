#include "hue4cpp/sensors/temperature_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	TemperatureSensor::TemperatureSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::Temperature) {
	}

	SensorType TemperatureSensor::getType() const {
		return SensorType::Temperature;
	}

	void TemperatureSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("temperature")) {
			auto temp_obj = delta["temperature"];
			int temp_raw       = json_utils::getValueOr<int>(temp_obj, "temperature",       0);
			_temperature       = temp_raw / 100.0f;
			_temperature_valid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", _temperature_valid);
			NotifyPropertyChanged<&TemperatureSensor::Temperature>();
			NotifyPropertyChanged<&TemperatureSensor::TemperatureValid>();
		}
	}

} // namespace hue4cpp
