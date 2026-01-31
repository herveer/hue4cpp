#pragma once

#include "sensor_base.h"

/**
 * @file motion_sensor.h
 * @brief Motion sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a motion sensor
 * 
 * Motion sensors detect movement and provide motion state information.
 */
class MotionSensor : public Sensor {
public:
    /**
     * @brief Construct a MotionSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    MotionSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::Motion
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current motion state
     * @return MotionState with motion detection status
     */
    MotionState getMotionState() const;
};

} // namespace hue4cpp
