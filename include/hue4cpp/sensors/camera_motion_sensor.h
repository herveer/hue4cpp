#pragma once

#include "sensor_base.h"

/**
 * @file camera_motion_sensor.h
 * @brief Camera motion sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a camera motion sensor
 * 
 * Camera motion sensors detect movement using camera vision rather than PIR.
 */
class CameraMotionSensor : public Sensor {
public:
    /**
     * @brief Construct a CameraMotionSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    CameraMotionSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::CameraMotion
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current camera motion state
     * @return CameraMotionState with motion detection status
     */
    CameraMotionState getCameraMotionState() const;
};

} // namespace hue4cpp
