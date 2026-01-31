#include "hue4cpp/sensors/relative_rotary_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	namespace {
		// Helper function to parse rotation direction from string
		RotationDirection parseRotationDirection(const std::string& direction_str) {
			if (direction_str == "clock_wise") return RotationDirection::ClockWise;
			if (direction_str == "counter_clock_wise") return RotationDirection::CounterClockWise;
			return RotationDirection::Unknown;
		}

		// Helper function to parse button event from string
		ButtonEvent parseButtonEvent(const std::string& event_str) {
			if (event_str == "initial_press") return ButtonEvent::InitialPress;
			if (event_str == "short_release") return ButtonEvent::ShortRelease;
			if (event_str == "long_release") return ButtonEvent::LongRelease;
			if (event_str == "long_press") return ButtonEvent::LongPress;
			if (event_str == "double_short_release") return ButtonEvent::DoubleShortRelease;
			if (event_str == "repeat") return ButtonEvent::Repeat;
			return ButtonEvent::Unknown;
		}
	}

	RelativeRotarySensor::RelativeRotarySensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::RelativeRotary) {
	}

	SensorType RelativeRotarySensor::getType() const {
		return SensorType::RelativeRotary;
	}

	RelativeRotaryState RelativeRotarySensor::getRelativeRotaryState() const {
		// Extract relative rotary state from cached JSON
		auto extractRelativeRotary = [](const std::string& state_json) -> RelativeRotaryState {
			RelativeRotaryState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("relative_rotary") && state["relative_rotary"].is_object()) {
						auto rotary_obj = state["relative_rotary"];
						
						// Get rotation data
						if (rotary_obj.contains("last_event") && rotary_obj["last_event"].is_object()) {
							auto event_obj = rotary_obj["last_event"];
							result.steps = json_utils::getValueOr<int32_t>(event_obj, "rotation", 0);
							std::string direction_str = json_utils::getValueOr<std::string>(event_obj, "direction", "");
							result.direction = parseRotationDirection(direction_str);
							std::string action_str = json_utils::getValueOr<std::string>(event_obj, "action", "");
							result.action = parseButtonEvent(action_str);
						}
						
						result.event_sequence = json_utils::getValueOr<uint32_t>(rotary_obj, "event_sequence", 0);
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
			return RelativeRotaryState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), false);
		auto rotary_state = extractRelativeRotary(state_json);
		if (rotary_state.direction != RotationDirection::Unknown || rotary_state.action != ButtonEvent::Unknown) {
			return rotary_state;
		}

		// Try refreshing cache
		state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), true);
		return extractRelativeRotary(state_json);
	}

} // namespace hue4cpp
