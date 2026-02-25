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

	void GeolocationSensor::notifyStateProperties(const nlohmann::json& delta) {
		if (delta.contains("is_configured")) {
			_is_configured = json_utils::getValueOr<bool>(delta, "is_configured", _is_configured);
			NotifyPropertyChanged<&GeolocationSensor::IsConfigured>();
		}
	}

} // namespace hue4cpp
