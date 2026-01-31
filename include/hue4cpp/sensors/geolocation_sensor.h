#pragma once

#include "sensor_base.h"

/**
 * @file geolocation_sensor.h
 * @brief Geolocation sensor class (geofencing)
 */

namespace hue4cpp {

/**
 * @brief Represents a geolocation sensor
 * 
 * Geolocation sensors provide geofencing capabilities,
 * detecting when users enter or leave a defined area.
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
    
    /**
     * @brief Get current geolocation state
     * @return GeolocationState with configuration status
     */
    GeolocationState getGeolocationState() const;
};

} // namespace hue4cpp
