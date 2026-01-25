#include "hue4cpp/color_utils.h"
#include <cmath>
#include <algorithm>
#include <cctype>

namespace hue4cpp {

namespace color_utils {

XYColor rgbToXy(const RGBColor& rgb) {
    // Normalize RGB values to 0-1 range
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    // Apply gamma correction
    auto gammaCorrect = [](float value) -> float {
        return (value > 0.04045f) ? 
            std::pow((value + 0.055f) / 1.055f, 2.4f) : 
            (value / 12.92f);
    };
    
    r = gammaCorrect(r);
    g = gammaCorrect(g);
    b = gammaCorrect(b);
    
    // Convert to XYZ using Wide RGB D65 conversion
    float X = r * 0.664511f + g * 0.154324f + b * 0.162028f;
    float Y = r * 0.283881f + g * 0.668433f + b * 0.047685f;
    float Z = r * 0.000088f + g * 0.072310f + b * 0.986039f;
    
    float sum = X + Y + Z;
    
    // Avoid division by zero
    if (sum < 0.0001f) {
        return XYColor(0.0f, 0.0f);
    }
    
    float x = X / sum;
    float y = Y / sum;
    
    // Clamp values to valid range
    x = std::max(0.0f, std::min(1.0f, x));
    y = std::max(0.0f, std::min(1.0f, y));
    
    return XYColor(x, y);
}

RGBColor xyToRgb(const XYColor& xy, uint8_t brightness) {
    // Calculate z from x and y
    float z = 1.0f - xy.x - xy.y;
    
    // Calculate Y from brightness (0-100 to 0-1)
    float Y = brightness / 100.0f;
    
    // Calculate X and Z from x, y, z and Y
    float X = (Y / xy.y) * xy.x;
    float Z = (Y / xy.y) * z;
    
    // Convert from XYZ to RGB using Wide RGB D65
    float r = X *  1.656492f - Y * 0.354851f - Z * 0.255038f;
    float g = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f;
    float b = X *  0.051713f - Y * 0.121364f + Z * 1.011530f;
    
    // Apply reverse gamma correction
    auto reverseGamma = [](float value) -> float {
        return (value <= 0.0031308f) ? 
            12.92f * value : 
            1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    };
    
    r = reverseGamma(r);
    g = reverseGamma(g);
    b = reverseGamma(b);
    
    // Clamp to 0-1 range and convert to 0-255
    r = std::max(0.0f, std::min(1.0f, r)) * 255.0f;
    g = std::max(0.0f, std::min(1.0f, g)) * 255.0f;
    b = std::max(0.0f, std::min(1.0f, b)) * 255.0f;
    
    return RGBColor(r, g, b);
}

RGBColor hsvToRgb(float h, float s, float v) {
    // Normalize inputs
    h = std::fmod(h, 360.0f);
    if (h < 0) h += 360.0f;
    s = std::max(0.0f, std::min(100.0f, s)) / 100.0f;
    v = std::max(0.0f, std::min(100.0f, v)) / 100.0f;
    
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 60.0f) {
        r = c; g = x; b = 0;
    } else if (h < 120.0f) {
        r = x; g = c; b = 0;
    } else if (h < 180.0f) {
        r = 0; g = c; b = x;
    } else if (h < 240.0f) {
        r = 0; g = x; b = c;
    } else if (h < 300.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return RGBColor(
        (r + m) * 255.0f,
        (g + m) * 255.0f,
        (b + m) * 255.0f
    );
}

void rgbToHsv(const RGBColor& rgb, float& h, float& s, float& v) {
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float delta = max - min;
    
    // Calculate value
    v = max * 100.0f;
    
    // Calculate saturation
    if (max < 0.0001f) {
        s = 0.0f;
        h = 0.0f;
        return;
    }
    s = (delta / max) * 100.0f;
    
    // Calculate hue
    if (delta < 0.0001f) {
        h = 0.0f;
    } else if (max == r) {
        h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
    } else if (max == g) {
        h = 60.0f * (((b - r) / delta) + 2.0f);
    } else {
        h = 60.0f * (((r - g) / delta) + 4.0f);
    }
    
    if (h < 0.0f) {
        h += 360.0f;
    }
}

} // namespace color_utils

namespace colors {

// Static color map for name lookup
static const std::map<std::string, RGBColor> createColorMap() {
    return {
        // Primary colors
        {"red", Red},
        {"green", Green},
        {"blue", Blue},
        
        // Secondary colors
        {"yellow", Yellow},
        {"cyan", Cyan},
        {"magenta", Magenta},
        
        // Grayscale
        {"white", White},
        {"black", Black},
        
        // Common colors
        {"orange", Orange},
        {"purple", Purple},
        {"pink", Pink},
        {"lightblue", LightBlue},
        {"lightgreen", LightGreen},
        
        // Warm colors
        {"warmwhite", WarmWhite},
        {"candlelight", Candlelight},
        
        // Cool colors
        {"coolwhite", CoolWhite},
        {"daylight", Daylight}
    };
}

std::optional<RGBColor> getColorByName(const std::string& name) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Use function-local static to avoid repeated construction
    static const auto color_map = createColorMap();
    
    auto it = color_map.find(lower_name);
    if (it != color_map.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<std::string> getColorNames() {
    return {
        "Red", "Green", "Blue",
        "Yellow", "Cyan", "Magenta",
        "White", "Black",
        "Orange", "Purple", "Pink",
        "LightBlue", "LightGreen",
        "WarmWhite", "Candlelight",
        "CoolWhite", "Daylight"
    };
}

} // namespace colors

} // namespace hue4cpp
