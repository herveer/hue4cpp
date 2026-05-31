#include "hue4cpp/sensors/geolocation_sensor.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	GeolocationSensor::GeolocationSensor(const std::string& id, Bridge* bridge)
		: Sensor(id, bridge, SensorType::Geolocation) {
	}

	SensorType GeolocationSensor::getType() const {
		return SensorType::Geolocation;
	}

	void GeolocationSensor::initFromJson(const nlohmann::json& json) {
		Sensor::initFromJson(json);
		if (json.contains("is_configured")) {
			auto newVal = json_utils::getValueOr<bool>(json, "is_configured", _is_configured);
			SetPropertyValueAndNotify<&GeolocationSensor::IsConfigured>(_is_configured, newVal);
		}
	}

	void GeolocationSensor::notifyStateProperties(const nlohmann::json& delta) {
		try {
			if (delta.contains("is_configured")) {
				auto newVal = json_utils::getValueOr<bool>(delta, "is_configured", _is_configured);
				SetPropertyValueAndNotify<&GeolocationSensor::IsConfigured>(_is_configured, newVal);
			}
		}
		catch (...) {
			SetPropertyValueAndNotify<&GeolocationSensor::IsConfigured>(_is_configured, _is_configured);
		}
	}

} // namespace hue4cpp
