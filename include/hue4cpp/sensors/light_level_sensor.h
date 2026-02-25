#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file light_level_sensor.h
 * @brief Light level sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a light level sensor
 */
class LightLevelSensor : public Sensor {
public:
    /**
     * @brief Construct a LightLevelSensor
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    LightLevelSensor(const std::string& id, Bridge* bridge);

    /**
     * @brief Get the sensor type
     * @return SensorType::LightLevel
     */
    SensorType getType() const override;

    /** @brief Illuminance in raw units (logarithmic scale) (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<uint32_t> LightLevel{
        [this]() { return _light_level; }
    };

    /** @brief Whether the light level reading is valid (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> LightLevelValid{
        [this]() { return _light_level_valid; }
    };

private:
    uint32_t _light_level       = 0;
    bool     _light_level_valid = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
