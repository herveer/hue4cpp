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

	void BellButtonSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("button") && json["button"].is_object()) {
			auto button_obj = json["button"];
			std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
			auto newEvent = parseButtonEvent(event_str);
			SetPropertyValueAndNotify<&BellButtonSensor::LastEvent>(_last_event, newEvent);
			auto newSeq = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
			SetPropertyValueAndNotify<&BellButtonSensor::EventSequence>(_event_sequence, newSeq);
		}
	}

	void BellButtonSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("button")) {
				auto button_obj = delta["button"];
				std::string event_str = json_utils::getValueOr<std::string>(button_obj, "last_event", "");
				auto newEvent = parseButtonEvent(event_str);
				SetPropertyValueAndNotify<&BellButtonSensor::LastEvent>(_last_event, newEvent);
				auto newSeq = json_utils::getValueOr<uint32_t>(button_obj, "event_sequence", _event_sequence);
				SetPropertyValueAndNotify<&BellButtonSensor::EventSequence>(_event_sequence, newSeq);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&BellButtonSensor::LastEvent>(_last_event, _last_event);
			SetPropertyValueAndNotify<&BellButtonSensor::EventSequence>(_event_sequence, _event_sequence);
		}
	}

} // namespace hue4cpp
