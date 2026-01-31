#pragma once

#include "sensor_base.h"

/**
 * @file bell_button_sensor.h
 * @brief Bell button sensor class (doorbell)
 */

namespace hue4cpp {

/**
 * @brief Represents a bell button sensor (doorbell)
 * 
 * Bell button sensors capture doorbell button press events.
 */
class BellButtonSensor : public Sensor {
public:
    /**
     * @brief Construct a BellButtonSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    BellButtonSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::BellButton
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current bell button state
     * @return BellButtonState with last press event
     */
    BellButtonState getBellButtonState() const;
};

} // namespace hue4cpp
