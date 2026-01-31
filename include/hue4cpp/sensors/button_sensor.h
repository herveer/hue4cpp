#pragma once

#include "sensor_base.h"

/**
 * @file button_sensor.h
 * @brief Button sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a button sensor (dimmer switch, tap dial, etc.)
 * 
 * Button sensors capture button press events including short/long presses,
 * releases, and multi-button configurations.
 */
class ButtonSensor : public Sensor {
public:
    /**
     * @brief Construct a ButtonSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    ButtonSensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::Button
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current button state
     * @return ButtonState with last event and button information
     */
    ButtonState getButtonState() const;
};

} // namespace hue4cpp
