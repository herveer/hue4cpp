#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file temperature_sensor.h
 * @brief Temperature sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a temperature sensor
 */
class TemperatureSensor : public Sensor {
public:
    TemperatureSensor(const std::string& id, Bridge* bridge);

    SensorType getType() const override;

    /**
     * @brief Initialize temperature sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Current temperature in degrees Celsius (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<float> Temperature{
        [this]() { return _temperature; }
    };

    /** @brief Whether the temperature reading is valid (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> TemperatureValid{
        [this]() { return _temperature_valid; }
    };

private:
    float _temperature       = 0.0f;
    bool  _temperature_valid = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
