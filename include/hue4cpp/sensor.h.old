#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <memory>

/**
 * @file sensor.h
 * @brief Sensor monitoring and management
 */

namespace hue4cpp {

// Forward declaration
class Bridge;

/**
 * @brief Represents a Hue sensor
 * 
 * The Sensor class provides access to various sensor types including
 * motion sensors, temperature sensors, light level sensors, and buttons.
 */
class Sensor {
public:
    /**
     * @brief Default constructor
     */
    Sensor();
    
    /**
     * @brief Construct a Sensor with ID and parent bridge
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     */
    Sensor(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Destructor
     */
    ~Sensor();
    
    // Allow copying and moving
    Sensor(const Sensor&);
    Sensor& operator=(const Sensor&);
    Sensor(Sensor&&) noexcept;
    Sensor& operator=(Sensor&&) noexcept;
    
    /**
     * @brief Get the sensor's unique identifier
     * @return Sensor ID
     */
    std::string getId() const;
    
    /**
     * @brief Get the sensor's type
     * @return SensorType
     */
    SensorType getType() const;
    
    /**
     * @brief Check if sensor is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;
    
    /**
     * @brief Get motion sensor state (if this is a motion sensor)
     * @return MotionState if available, nullopt otherwise
     */
    std::optional<MotionState> getMotionState() const;
    
    /**
     * @brief Get temperature sensor reading (if this is a temperature sensor)
     * @return TemperatureState if available, nullopt otherwise
     */
    std::optional<TemperatureState> getTemperatureState() const;
    
    /**
     * @brief Get light level sensor reading (if this is a light level sensor)
     * @return LightLevelState if available, nullopt otherwise
     */
    std::optional<LightLevelState> getLightLevelState() const;
    
    /**
     * @brief Get button sensor state (if this is a button sensor)
     * @return ButtonState if available, nullopt otherwise
     */
    std::optional<ButtonState> getButtonState() const;
    
    /**
     * @brief Refresh sensor state from bridge
     * @return Result indicating success or failure
     */
    Result<void> refresh();
    
    /**
     * @brief Update sensor state from JSON data
     * @param json JSON object containing sensor data from API
     * @note This is an internal method used by Bridge to populate sensor data
     */
    void updateFromJson(const nlohmann::json& json);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    friend class Bridge;
};

} // namespace hue4cpp
