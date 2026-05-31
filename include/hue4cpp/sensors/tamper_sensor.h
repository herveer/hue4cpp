#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file tamper_sensor.h
 * @brief Tamper detection sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a tamper detection sensor
 */
class TamperSensor : public Sensor {
public:
    TamperSensor(const std::string& id, Bridge* bridge);

    SensorType getType() const override;

    /**
     * @brief Initialize tamper sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Whether physical tampering has been detected (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> Tampered{
        [this]() { return _tampered; }
    };

    /** @brief Whether the tamper reading is valid (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> TamperValid{
        [this]() { return _tamper_valid; }
    };

private:
    bool _tampered     = false;
    bool _tamper_valid = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
