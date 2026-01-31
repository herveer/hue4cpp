#pragma once

#include "../types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

/**
 * @file sensor_base.h
 * @brief Base class for all sensor types
 */

namespace hue4cpp {

// Forward declaration
class Bridge;

/**
 * @brief Abstract base class for all Hue sensors
 * 
 * This class provides common functionality for all sensor types.
 * Specific sensor types derive from this class and provide type-specific methods.
 */
class Sensor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~Sensor();
    
    // Prevent copying (use smart pointers for polymorphic storage)
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    
    // Allow moving
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
    virtual SensorType getType() const = 0;
    
    /**
     * @brief Check if sensor is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;
    
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
    
protected:
    /**
     * @brief Protected constructor - only derived classes can construct
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     * @param type Sensor type
     */
    Sensor(const std::string& id, Bridge* bridge, SensorType type);
    
    /**
     * @brief Get the resource type string for API calls
     * @return Resource type string (e.g., "motion", "temperature")
     */
    std::string getResourceTypeString() const;
    
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    friend class Bridge;
};

/**
 * @brief Factory function to create appropriate sensor from JSON
 * @param json JSON object containing sensor data
 * @param bridge Pointer to parent bridge
 * @return Unique pointer to created sensor (nullptr if type unknown)
 */
std::unique_ptr<Sensor> createSensorFromJson(const nlohmann::json& json, Bridge* bridge);

} // namespace hue4cpp
