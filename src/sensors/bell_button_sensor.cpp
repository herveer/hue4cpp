#include "hue4cpp/sensors/bell_button_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	namespace {
		// Helper function to parse button event from string (same as ButtonSensor)
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

	BellButtonSensor::BellButtonSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::BellButton) {
	}

	SensorType BellButtonSensor::getType() const {
		return SensorType::BellButton;
	}

	BellButtonState BellButtonSensor::getBellButtonState() const {
		// Extract bell button state from cached JSON
		auto extractBellButton = [](const std::string& state_json) -> BellButtonState {
			BellButtonState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					if (state.contains("button") && state["button"].is_object()) {
						auto button_obj = state["button"];
						std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
						result.last_event = parseButtonEvent(event_str);
						result.event_sequence = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", 0);
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
			return BellButtonState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), false);
		auto bell_button_state = extractBellButton(state_json);
		if (bell_button_state.last_event != ButtonEvent::Unknown) {
			return bell_button_state;
		}

		// Try refreshing cache
		state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), true);
		return extractBellButton(state_json);
	}

} // namespace hue4cpp
