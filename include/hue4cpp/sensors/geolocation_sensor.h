#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file geolocation_sensor.h
 * @brief Geolocation sensor class (geofencing)
 */

namespace hue4cpp {

/**
 * @brief Represents a geolocation sensor
 *
 * Geolocation sensors provide geofencing capabilities as a reactive
 * ReadonlyProperty. Subscribe to OnStateChanged for real-time updates.
 */
class GeolocationSensor : public Sensor {
public:
    /**
     * @brief Construct a GeolocationSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    GeolocationSensor(const std::string& id, Bridge* bridge);

    /**
     * @brief Get the sensor type
     * @return SensorType::Geolocation
     */
    SensorType getType() const override;

    /** @brief Whether geofencing has been configured on the bridge (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> IsConfigured{
        [this]() { return _is_configured; }
    };

private:
    bool _is_configured = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
