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
		RotationAction parseRotationAction(const std::string& event_str) {
			if (event_str == "start") return RotationAction::Start;
			if (event_str == "repeat") return RotationAction::Repeat;
			return RotationAction::Unknown;
		}
	}

	RelativeRotarySensor::RelativeRotarySensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::RelativeRotary) {
	}

	SensorType RelativeRotarySensor::getType() const {
		return SensorType::RelativeRotary;
	}

	void RelativeRotarySensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		notifyStateProperties(json);
	}

	void RelativeRotarySensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (!delta.contains("relative_rotary") || !delta["relative_rotary"].is_object()) return;

			auto rotary_obj = delta["relative_rotary"];
			if (!rotary_obj.contains("last_event") || !rotary_obj["last_event"].is_object()) return;

			auto event_obj = rotary_obj["last_event"];
			auto rotationAction = parseRotationAction(json_utils::getValueOr<std::string>(event_obj, "action", ""));
			SetPropertyValueAndNotify<&RelativeRotarySensor::Action>(_rotationAction, rotationAction);

			if (!event_obj.contains("rotation") || !event_obj["rotation"].is_object()) return;
			auto rotation_obj = event_obj["rotation"];

			auto steps = json_utils::getValueOr<int32_t>(rotation_obj, "steps", _steps);
			auto duration = json_utils::getValueOr<int32_t>(rotation_obj, "duration", _duration);
			auto direction = parseRotationDirection(json_utils::getValueOr<std::string>(rotation_obj, "direction", ""));
			SetPropertyValueAndNotify<&RelativeRotarySensor::Steps>(_steps, steps);
			SetPropertyValueAndNotify<&RelativeRotarySensor::Duration>(_duration, duration);
			SetPropertyValueAndNotify<&RelativeRotarySensor::Direction>(_direction, direction);
		}
		catch (...) {
		}
	}
	} // namespace hue4cpp
