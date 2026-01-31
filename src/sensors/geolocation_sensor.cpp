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

	GeolocationState GeolocationSensor::getGeolocationState() const {
		// Extract geolocation state from cached JSON
		auto extractGeolocation = [](const std::string& state_json) -> GeolocationState {
			GeolocationState result;
			if (!state_json.empty()) {
				try {
					auto state = json_utils::parse(state_json);
					// Geolocation sensor has simple is_configured flag
					result.is_configured = json_utils::getValueOr<bool>(state, "is_configured", false);
				}
				catch (...) {
					// Return default-constructed state
				}
			}
			return result;
		};

		if (!getBridge()) {
			// No bridge - cannot get state, return default
			return GeolocationState();
		}

		// Ask bridge for sensor state (cache-first, API-fallback)
		std::string state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), false);
		auto geolocation_state = extractGeolocation(state_json);
		
		// For geolocation, we always try to get the latest state
		if (!state_json.empty()) {
			return geolocation_state;
		}

		// Try refreshing cache
		state_json = getBridge()->getSensorState(getId(), getResourceTypeString(), true);
		return extractGeolocation(state_json);
	}

} // namespace hue4cpp
