#include "hue4cpp/sensors/camera_motion_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	CameraMotionSensor::CameraMotionSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::CameraMotion) {
	}

	SensorType CameraMotionSensor::getType() const {
		return SensorType::CameraMotion;
	}

	CameraMotionState CameraMotionSensor::getCameraMotionState() const {
		// Extract camera motion state from cached JSON
		auto extractCameraMotion = [](const std::string& state_json) -> CameraMotionState {
			CameraMotionState result;
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

		if (!getBridge()) {
			// No bridge - cannot get state, return default
			return CameraMotionState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), false);
		auto camera_motion_state = extractCameraMotion(state_json);
		if (camera_motion_state.motion_valid) {
			return camera_motion_state;
		}

		// Try refreshing cache
		state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), true);
		return extractCameraMotion(state_json);
	}

} // namespace hue4cpp
