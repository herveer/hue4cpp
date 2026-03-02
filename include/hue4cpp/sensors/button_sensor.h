#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file button_sensor.h
 * @brief Button sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a button sensor (dimmer switch, tap dial, etc.)
 * 
 * Button sensors expose the last press event as a reactive ReadonlyProperty.
 * Subscribe to OnStateChanged for real-time updates.
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
     * @brief Initialize button sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Last button event (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<ButtonEvent> LastEvent{
        [this]() { return _last_event; }
    };

    /** @brief Button control ID on multi-button devices (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<uint32_t> ButtonId{
        [this]() { return _button_id; }
    };

    /** @brief Monotonically increasing event counter (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<uint32_t> EventSequence{
        [this]() { return _event_sequence; }
    };

private:
    ButtonEvent _last_event     = ButtonEvent::Unknown;
    uint32_t    _button_id      = 0;
    uint32_t    _event_sequence = 0;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
