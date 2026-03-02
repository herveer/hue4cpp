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

	void ButtonSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("button") && json["button"].is_object()) {
			auto button_obj = json["button"];
			std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
			auto newEvent = parseButtonEvent(event_str);
			SetPropertyValueAndNotify<&ButtonSensor::LastEvent>(_last_event, newEvent);
			auto newSeq = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
			SetPropertyValueAndNotify<&ButtonSensor::EventSequence>(_event_sequence, newSeq);
		}
		if (json.contains("metadata") && json["metadata"].is_object()) {
			auto newBtnId = json_utils::getValueOr<uint32_t>(json["metadata"], "control_id", _button_id);
			SetPropertyValueAndNotify<&ButtonSensor::ButtonId>(_button_id, newBtnId);
		}
	}

	void ButtonSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("button")) {
				auto button_obj = delta["button"];
				std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
				auto newEvent = parseButtonEvent(event_str);
				SetPropertyValueAndNotify<&ButtonSensor::LastEvent>(_last_event, newEvent);
				auto newSeq = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
				SetPropertyValueAndNotify<&ButtonSensor::EventSequence>(_event_sequence, newSeq);

				// Fire specific events based on the button event type
				switch (newEvent) {
				case ButtonEvent::InitialPress:
					Pressed.Notify();
					break;
				case ButtonEvent::ShortRelease:
				case ButtonEvent::LongRelease:
					Released.Notify();
					break;
				case ButtonEvent::LongPress:
				case ButtonEvent::DoubleShortRelease:
				case ButtonEvent::Repeat:
					Repeated.Notify();
					break;
				default:
					break;
				}
			}
			if (delta.contains("metadata") && delta["metadata"].contains("control_id")) {
				auto newBtnId = json_utils::getValueOr<uint32_t>(delta["metadata"], "control_id", _button_id);
				SetPropertyValueAndNotify<&ButtonSensor::ButtonId>(_button_id, newBtnId);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&ButtonSensor::LastEvent>(_last_event, _last_event);
			SetPropertyValueAndNotify<&ButtonSensor::ButtonId>(_button_id, _button_id);
			SetPropertyValueAndNotify<&ButtonSensor::EventSequence>(_event_sequence, _event_sequence);
		}
	}

} // namespace hue4cpp
