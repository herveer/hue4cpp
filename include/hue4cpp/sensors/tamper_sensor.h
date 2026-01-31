#pragma once

#include "sensor_base.h"

/**
 * @file tamper_sensor.h
 * @brief Tamper detection sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a tamper detection sensor
 * 
 * Tamper sensors detect when a device has been physically tampered with,
 * such as being removed from its mount or opened.
 */
class TamperSensor : public Sensor {
public:
    /**
     * @brief Construct a TamperSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    TamperSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::Tamper
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current tamper state
     * @return TamperState with tamper detection status
     */
    TamperState getTamperState() const;
};

} // namespace hue4cpp
