#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file motion_sensor.h
 * @brief Motion sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a motion sensor
 */
class MotionSensor : public Sensor {
public:
    MotionSensor(const std::string& id, Bridge* bridge);

    SensorType getType() const override;

    /**
     * @brief Initialize motion sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Whether motion is currently detected (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> Motion{
        [this]() { return _motion; }
    };

    /** @brief Whether the motion reading is valid (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> MotionValid{
        [this]() { return _motion_valid; }
    };

private:
    bool _motion       = false;
    bool _motion_valid = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
