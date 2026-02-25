#include "hue4cpp/sensors/button_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	namespace {
		ButtonEvent parseButtonEvent(const std::string& event_str) {
			if (event_str == "initial_press")       return ButtonEvent::InitialPress;
			if (event_str == "short_release")       return ButtonEvent::ShortRelease;
			if (event_str == "long_release")        return ButtonEvent::LongRelease;
			if (event_str == "long_press")          return ButtonEvent::LongPress;
			if (event_str == "double_short_release")return ButtonEvent::DoubleShortRelease;
			if (event_str == "repeat")              return ButtonEvent::Repeat;
			return ButtonEvent::Unknown;
		}
	}

	ButtonSensor::ButtonSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::Button) {
	}

	SensorType ButtonSensor::getType() const {
		return SensorType::Button;
	}

	void ButtonSensor::notifyStateProperties(const nlohmann::json& delta) {
		bool changed = false;

		if (delta.contains("button")) {
			auto button_obj  = delta["button"];
			std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
			_last_event      = parseButtonEvent(event_str);
			_event_sequence  = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
			changed = true;
		}
		if (delta.contains("metadata") && delta["metadata"].contains("control_id")) {
			_button_id = json_utils::getValueOr<uint32_t>(delta["metadata"], "control_id", _button_id);
			changed = true;
		}

		if (changed) {
			NotifyPropertyChanged<&ButtonSensor::LastEvent>();
			NotifyPropertyChanged<&ButtonSensor::ButtonId>();
			NotifyPropertyChanged<&ButtonSensor::EventSequence>();
		}
	}

} // namespace hue4cpp
