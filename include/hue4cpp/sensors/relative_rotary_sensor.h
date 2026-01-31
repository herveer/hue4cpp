#pragma once

#include "sensor_base.h"

/**
 * @file relative_rotary_sensor.h
 * @brief Relative rotary sensor class (dial/knob)
 */

namespace hue4cpp {

/**
 * @brief Represents a relative rotary sensor (dial/knob)
 * 
 * Relative rotary sensors capture rotation events from dials and knobs,
 * providing information about rotation direction and steps.
 */
class RelativeRotarySensor : public Sensor {
public:
    /**
     * @brief Construct a RelativeRotarySensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    RelativeRotarySensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Get the sensor type
     * @return SensorType::RelativeRotary
     */
    SensorType getType() const override;
    
    /**
     * @brief Get current relative rotary state
     * @return RelativeRotaryState with rotation information
     */
    RelativeRotaryState getRelativeRotaryState() const;
};

} // namespace hue4cpp
