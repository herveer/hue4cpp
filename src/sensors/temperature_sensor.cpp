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

	TemperatureState TemperatureSensor::getTemperatureState() const {
		// Extract temperature state from cached JSON
		auto extractTemperature = [](const std::string& state_json) -> TemperatureState {
			TemperatureState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("temperature") && state["temperature"].is_object()) {
						auto temp_obj = state["temperature"];
						// Temperature is in deci-degrees Celsius (divide by 100)
						int temp_raw = json_utils::getValueOr<int>(temp_obj, "temperature", 0);
						result.temperature = temp_raw / 100.0f;
						result.temperature_valid = json_utils::getValueOr<bool>(temp_obj, "temperature_valid", true);
					}
				}
				catch (...) {
					// Return default-constructed state
				}
			}
			return result;
		};

		if (!pImpl->bridge) {
			// No bridge - cannot get state, return default
			return TemperatureState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), false);
		auto temp_state = extractTemperature(state_json);
		if (temp_state.temperature_valid) {
			return temp_state;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), true);
		return extractTemperature(state_json);
	}

} // namespace hue4cpp
