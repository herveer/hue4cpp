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

	void TemperatureSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("temperature") && json["temperature"].is_object()) {
			auto temp_obj = json["temperature"];
			int temp_raw = json_utils::getValueOr<int>(temp_obj, "temperature", 0);
			auto newTemp = temp_raw / 100.0f;
			SetPropertyValueAndNotify<&TemperatureSensor::Temperature>(_temperature, newTemp);
			auto newValid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", _temperature_valid);
			SetPropertyValueAndNotify<&TemperatureSensor::TemperatureValid>(_temperature_valid, newValid);
		}
	}

	void TemperatureSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("temperature")) {
				auto temp_obj = delta["temperature"];
				int temp_raw = json_utils::getValueOr<int>(temp_obj, "temperature", 0);
				auto newTemp = temp_raw / 100.0f;
				SetPropertyValueAndNotify<&TemperatureSensor::Temperature>(_temperature, newTemp);
				auto newValid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", _temperature_valid);
				SetPropertyValueAndNotify<&TemperatureSensor::TemperatureValid>(_temperature_valid, newValid);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&TemperatureSensor::Temperature>(_temperature, _temperature);
			SetPropertyValueAndNotify<&TemperatureSensor::TemperatureValid>(_temperature_valid, _temperature_valid);
		}
	}

} // namespace hue4cpp
