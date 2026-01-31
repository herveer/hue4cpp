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

	MotionState MotionSensor::getMotionState() const {
		// Extract motion state from cached JSON
		auto extractMotion = [](const std::string& state_json) -> MotionState {
			MotionState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("motion") && state["motion"].is_object()) {
						auto motion_obj = state["motion"];
						result.motion = json_utils::getValueOr<bool>(motion_obj, "motion", false);
						result.motion_valid = json_utils::getValueOr<bool>(motion_obj, "motion_valid", false);
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
			return MotionState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), false);
		auto motion_state = extractMotion(state_json);
		if (motion_state.motion_valid) {
			return motion_state;
		}

		// Try refreshing cache
		state_json = pImpl->bridge->getSensorState(getId(), getResourceTypeString(), true);
		return extractMotion(state_json);
	}

} // namespace hue4cpp
