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

	TamperState TamperSensor::getTamperState() const {
		// Extract tamper state from cached JSON
		auto extractTamper = [](const std::string& state_json) -> TamperState {
			TamperState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("tamper") && state["tamper"].is_object()) {
						auto tamper_obj = state["tamper"];
						result.tampered = json_utils::getValueOr<bool>(tamper_obj, "tampered", false);
						result.tamper_valid = json_utils::getValueOr<bool>(tamper_obj, "tamper_valid", false);
					}
				}
				catch (...) {
					// Return default-constructed state
				}
			}
			return result;
		};

		if (!getBridge()) {
			// No bridge - cannot get state, return default
			return TamperState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), false);
		auto tamper_state = extractTamper(state_json);
		if (tamper_state.tamper_valid) {
			return tamper_state;
		}

		// Try refreshing cache
		state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), true);
		return extractTamper(state_json);
	}

} // namespace hue4cpp
