#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <memory>

/**
 * @file light.h
 * @brief Light control and management
 */

namespace hue4cpp {

// Forward declaration
class Bridge;

/**
 * @brief Represents a single Hue light
 * 
 * The Light class provides methods to control individual lights including
 * power state, brightness, color, and effects.
 */
class Light {
public:
    /**
     * @brief Default constructor
     */
    Light();
    
    /**
     * @brief Construct a Light with ID and parent bridge
     * @param id Light unique identifier
     * @param bridge Pointer to parent bridge
     */
    Light(const std::string& id, Bridge* bridge);
    
    /**
     * @brief Destructor
     */
    ~Light();
    
    // Allow copying and moving
    Light(const Light&);
    Light& operator=(const Light&);
    Light(Light&&) noexcept;
    Light& operator=(Light&&) noexcept;
    
    /**
     * @brief Get the light's unique identifier
     * @return Light ID
     */
    std::string getId() const;
    
    /**
     * @brief Get the light's name
     * @return Light name
     */
    std::string getName() const;
    
    /**
     * @brief Get the light's capabilities
     * @return LightCapabilities structure
     */
    LightCapabilities getCapabilities() const;
    
    /**
     * @brief Check if light is currently on
     * @return true if on, false if off
     */
    bool isOn() const;
    
    /**
     * @brief Turn the light on
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> turnOn(TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Turn the light off
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> turnOff(TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Toggle the light on/off
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> toggle(TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Get current brightness level
     * @return Brightness (0-100), or nullopt if not available
     */
    std::optional<uint8_t> getBrightness() const;
    
    /**
     * @brief Set brightness level
     * @param brightness Brightness level (0-100)
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> setBrightness(uint8_t brightness,
                               TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Get current color in XY color space
     * @return XYColor, or nullopt if not available
     */
    std::optional<XYColor> getColor() const;
    
    /**
     * @brief Set color using XY color space
     * @param color XY color coordinates
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> setColor(const XYColor& color,
                          TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Set color using RGB values
     * @param color RGB color
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     * @note RGB will be converted to XY using the light's color gamut
     */
    Result<void> setColor(const RGBColor& color,
                          TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Set color using separate RGB values
     * @param r Red (0-255)
     * @param g Green (0-255)
     * @param b Blue (0-255)
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> setColor(float r, float g, float b,
                          TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Get current color temperature
     * @return ColorTemperature, or nullopt if not available
     */
    std::optional<ColorTemperature> getColorTemperature() const;
    
    /**
     * @brief Set color temperature
     * @param temperature Color temperature
     * @param transition Transition time (default: 400ms)
     * @return Result indicating success or failure
     */
    Result<void> setColorTemperature(const ColorTemperature& temperature,
                                     TransitionTime transition = std::chrono::milliseconds(400));
    
    /**
     * @brief Trigger alert effect (light blinks)
     * @param long_alert If true, use long alert; otherwise short alert
     * @return Result indicating success or failure
     */
    Result<void> alert(bool long_alert = false);
    
    /**
     * @brief Refresh light state from bridge
     * @return Result indicating success or failure
     */
    Result<void> refresh();
    
    /**
     * @brief Update light state from JSON data
     * @param json JSON object containing light data from API
     * @note This is an internal method used by Bridge to populate light data
     */
    void updateFromJson(const nlohmann::json& json);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    friend class Bridge;
};

} // namespace hue4cpp
