#pragma once

#include "sensor_base.h"
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file camera_motion_sensor.h
 * @brief Camera motion sensor class
 */

namespace hue4cpp {

/**
 * @brief Represents a camera motion sensor
 */
class CameraMotionSensor : public Sensor {
public:
    CameraMotionSensor(const std::string& id, Bridge* bridge);

    SensorType getType() const override;

    /**
     * @brief Initialize camera motion sensor state from JSON data
     * @param json JSON object containing sensor data from API
     */
    void initFromJson(const nlohmann::json& json) override;

    /** @brief Whether motion is currently detected by the camera (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> CameraMotion{
        [this]() { return _motion; }
    };

    /** @brief Whether the camera motion reading is valid (reactive, read-only) */
    ReactiveLitepp::ReadonlyProperty<bool> CameraMotionValid{
        [this]() { return _motion_valid; }
    };

	/** @brief Events fired when motion is detected or cleared */
    ReactiveLitepp::Event<> MotionDetected;
	ReactiveLitepp::Event<> MotionCleared;

private:
    bool _motion       = false;
    bool _motion_valid = false;

    void notifyStateProperties(const nlohmann::json& delta) override;

    friend class Bridge;
};

} // namespace hue4cpp
