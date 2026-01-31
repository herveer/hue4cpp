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

	LightLevelState LightLevelSensor::getLightLevelState() const {
		// Extract light level state from cached JSON
		auto extractLightLevel = [](const std::string& state_json) -> LightLevelState {
			LightLevelState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("light") && state["light"].is_object()) {
						auto light_obj = state["light"];
						result.light_level = json_utils::getValueOr<uint32_t>(light_obj, "light_level", 0);
						result.light_level_valid = json_utils::getValueOr<bool>(light_obj, "light_level_valid", false);
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
			return LightLevelState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), false);
		auto light_state = extractLightLevel(state_json);
		if (light_state.light_level_valid) {
			return light_state;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), true);
		return extractLightLevel(state_json);
	}

} // namespace hue4cpp
