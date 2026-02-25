#include "hue4cpp/sensors/bell_button_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	namespace {
		ButtonEvent parseButtonEvent(const std::string& event_str) {
			if (event_str == "initial_press")        return ButtonEvent::InitialPress;
			if (event_str == "short_release")        return ButtonEvent::ShortRelease;
			if (event_str == "long_release")         return ButtonEvent::LongRelease;
			if (event_str == "long_press")           return ButtonEvent::LongPress;
			if (event_str == "double_short_release") return ButtonEvent::DoubleShortRelease;
			if (event_str == "repeat")               return ButtonEvent::Repeat;
			return ButtonEvent::Unknown;
		}
	}

	BellButtonSensor::BellButtonSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::BellButton) {
	}

	SensorType BellButtonSensor::getType() const {
		return SensorType::BellButton;
	}

	void BellButtonSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("button")) {
			auto button_obj = delta["button"];
			std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
			_last_event     = parseButtonEvent(event_str);
			_event_sequence = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
			NotifyPropertyChanged<&BellButtonSensor::LastEvent>();
			NotifyPropertyChanged<&BellButtonSensor::EventSequence>();
		}
	}

} // namespace hue4cpp
