#pragma once

#include "sensor_base.h"

/**
 * @file temperature_sensor.h
 * @brief Temperature sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a temperature sensor
 * 
 * Temperature sensors measure ambient temperature in degrees Celsius.
 */
class TemperatureSensor : public Sensor {
public:
    /**
     * @brief Construct a TemperatureSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    TemperatureSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::Temperature
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current temperature reading
     * @return TemperatureState with temperature in degrees Celsius
     */
    TemperatureState getTemperatureState() const;
};

} // namespace hue4cpp
