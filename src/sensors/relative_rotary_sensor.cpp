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

	void RelativeRotarySensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("relative_rotary")) {
			auto rotary_obj = delta["relative_rotary"];
			if (rotary_obj.contains("last_event") && rotary_obj["last_event"].is_object()) {
				auto event_obj = rotary_obj["last_event"];
				_steps     = json_utils::getValueOr<int32_t>(event_obj, "rotation",  _steps);
				_direction = parseRotationDirection(json_utils::getValueOr<std::string>(event_obj, "direction", ""));
				_action    = parseButtonEvent(json_utils::getValueOr<std::string>(event_obj, "action", ""));
			}
			_event_sequence = json_utils::getValueOr<uint32_t>(rotary_obj, "event_sequence", _event_sequence);
			NotifyPropertyChanged<&RelativeRotarySensor::Steps>();
			NotifyPropertyChanged<&RelativeRotarySensor::Direction>();
			NotifyPropertyChanged<&RelativeRotarySensor::Action>();
			NotifyPropertyChanged<&RelativeRotarySensor::EventSequence>();
		}
	}

} // namespace hue4cpp
