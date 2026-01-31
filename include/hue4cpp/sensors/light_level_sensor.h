#pragma once

#include "sensor_base.h"

/**
 * @file light_level_sensor.h
 * @brief Light level sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a light level sensor
 * 
 * Light level sensors measure ambient illuminance.
 */
class LightLevelSensor : public Sensor {
public:
    /**
     * @brief Construct a LightLevelSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    LightLevelSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::LightLevel
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current light level reading
     * @return LightLevelState with illuminance measurement
     */
    LightLevelState getLightLevelState() const;
};

} // namespace hue4cpp
