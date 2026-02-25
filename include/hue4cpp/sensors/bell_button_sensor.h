#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file bell_button_sensor.h
 * @brief Bell button sensor class (doorbell)
 */

namespace hue4cpp {

/**
 * @brief Represents a bell button sensor (doorbell)
 * 
 * Bell button sensors expose the last doorbell press as a reactive
 * ReadonlyProperty. Subscribe to OnStateChanged for real-time updates.
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

    /** @brief Last doorbell button event (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<ButtonEvent> LastEvent{
        [this]() { return _last_event; }
    };

    /** @brief Monotonically increasing event counter (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<uint32_t> EventSequence{
        [this]() { return _event_sequence; }
    };

private:
    ButtonEvent _last_event     = ButtonEvent::Unknown;
    uint32_t    _event_sequence = 0;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
