#pragma once

#include "types.h"
#include <string>
#include <map>
#include <vector>

/**
 * @file color_utils.h
 * @brief Color conversion utilities and preset color palettes
 */

namespace hue4cpp {

/**
 * @brief Color conversion utilities
 */
namespace color_utils {

    /**
     * @brief Convert RGB color to XY color space
     * @param rgb RGB color (0-255 per component)
     * @return XY color in CIE 1931 color space
     * @note Uses Wide RGB D65 gamut for conversion
     */
    XYColor rgbToXy(const RGBColor& rgb);
    
    /**
     * @brief Convert XY color to approximate RGB
     * @param xy XY color coordinates
     * @param brightness Optional brightness (0-100)
     * @return Approximate RGB color
     * @note This is an approximation and may not be exact due to gamut limitations
     */
    RGBColor xyToRgb(const XYColor& xy, uint8_t brightness = 100);
    
    /**
     * @brief Convert HSV color to RGB
     * @param h Hue (0-360 degrees)
     * @param s Saturation (0-100%)
     * @param v Value/Brightness (0-100%)
     * @return RGB color
     */
    RGBColor hsvToRgb(float h, float s, float v);
    
    /**
     * @brief Convert RGB to HSV
     * @param rgb RGB color
     * @param[out] h Hue (0-360 degrees)
     * @param[out] s Saturation (0-100%)
     * @param[out] v Value/Brightness (0-100%)
     */
    void rgbToHsv(const RGBColor& rgb, float& h, float& s, float& v);
    
} // namespace color_utils

/**
 * @brief Preset color palettes for common colors
 */
namespace colors {
    
    // Primary colors
    const RGBColor Red{255.0f, 0.0f, 0.0f};
    const RGBColor Green{0.0f, 255.0f, 0.0f};
    const RGBColor Blue{0.0f, 0.0f, 255.0f};
    
    // Secondary colors
    const RGBColor Yellow{255.0f, 255.0f, 0.0f};
    const RGBColor Cyan{0.0f, 255.0f, 255.0f};
    const RGBColor Magenta{255.0f, 0.0f, 255.0f};
    
    // Grayscale
    const RGBColor White{255.0f, 255.0f, 255.0f};
    const RGBColor Black{0.0f, 0.0f, 0.0f};
    
    // Common colors
    const RGBColor Orange{255.0f, 165.0f, 0.0f};
    const RGBColor Purple{128.0f, 0.0f, 128.0f};
    const RGBColor Pink{255.0f, 192.0f, 203.0f};
    const RGBColor LightBlue{173.0f, 216.0f, 230.0f};
    const RGBColor LightGreen{144.0f, 238.0f, 144.0f};
    
    // Warm colors
    const RGBColor WarmWhite{255.0f, 244.0f, 229.0f};  // ~2700K
    const RGBColor Candlelight{255.0f, 147.0f, 41.0f};  // ~2000K
    
    // Cool colors
    const RGBColor CoolWhite{201.0f, 226.0f, 255.0f};  // ~6500K
    const RGBColor Daylight{255.0f, 251.0f, 245.0f};   // ~5500K
    
    /**
     * @brief Get a color by name
     * @param name Color name (case-insensitive)
     * @return RGB color, or std::nullopt if name not found
     */
    std::optional<RGBColor> getColorByName(const std::string& name);
    
    /**
     * @brief Get all available color names
     * @return Vector of color names
     */
    std::vector<std::string> getColorNames();
    
} // namespace colors

} // namespace hue4cpp
