#pragma once

#include <string>
#include <optional>
#include <chrono>

/**
 * @file types.h
 * @brief Common types and definitions used throughout hue4cpp
 */

namespace hue4cpp {

/**
 * @brief RGB color representation
 */
struct RGBColor {
    float r; ///< Red component (0.0 - 255.0)
    float g; ///< Green component (0.0 - 255.0)
    float b; ///< Blue component (0.0 - 255.0)
    
    RGBColor(float red = 0.0f, float green = 0.0f, float blue = 0.0f)
        : r(red), g(green), b(blue) {}
};

/**
 * @brief XY color representation (CIE 1931 color space)
 */
struct XYColor {
    float x; ///< X coordinate (0.0 - 1.0)
    float y; ///< Y coordinate (0.0 - 1.0)
    
    XYColor(float x_coord = 0.0f, float y_coord = 0.0f)
        : x(x_coord), y(y_coord) {}
};

/**
 * @brief Color temperature in mireds
 */
struct ColorTemperature {
    uint16_t mireds; ///< Color temperature in mireds (153-500)
    
    explicit ColorTemperature(uint16_t value = 153)
        : mireds(value) {}
    
    /**
     * @brief Create from Kelvin temperature
     * @param kelvin Temperature in Kelvin (2000-6500)
     * @return ColorTemperature in mireds
     */
    static ColorTemperature fromKelvin(uint16_t kelvin) {
        return ColorTemperature(static_cast<uint16_t>(1000000 / kelvin));
    }
    
    /**
     * @brief Convert to Kelvin temperature
     * @return Temperature in Kelvin
     */
    uint16_t toKelvin() const {
        return static_cast<uint16_t>(1000000 / mireds);
    }
};

/**
 * @brief Light capabilities
 */
struct LightCapabilities {
    bool on_off;            ///< Can turn on/off
    bool brightness;        ///< Can adjust brightness
    bool color;             ///< Can set color
    bool color_temperature; ///< Can set color temperature
    bool effects;           ///< Supports effects
};

/**
 * @brief Transition time for light changes
 */
using TransitionTime = std::chrono::milliseconds;

/**
 * @brief Bridge connection information
 */
struct BridgeInfo {
    std::string ip_address;     ///< IP address of the bridge
    std::string id;             ///< Unique bridge identifier
    std::string name;           ///< User-friendly bridge name
    std::string model_id;       ///< Bridge model identifier
    std::string sw_version;     ///< Software version
    
    BridgeInfo() = default;
    BridgeInfo(const std::string& ip, const std::string& bridge_id)
        : ip_address(ip), id(bridge_id) {}
};

/**
 * @brief Error codes for hue4cpp operations
 */
enum class ErrorCode {
    Success = 0,
    NetworkError,
    AuthenticationRequired,
    AuthenticationFailed,
    InvalidRequest,
    ResourceNotFound,
    InvalidParameter,
    BridgeNotReachable,
    TimeoutError,
    UnknownError
};

/**
 * @brief Result type for operations that can fail
 * @tparam T The type of the result value
 */
template<typename T>
struct Result {
    std::optional<T> value;
    ErrorCode error;
    std::string error_message;
    
    Result() : error(ErrorCode::Success) {}
    explicit Result(T val) : value(std::move(val)), error(ErrorCode::Success) {}
    Result(ErrorCode err, const std::string& msg = "")
        : error(err), error_message(msg) {}
    
    bool isSuccess() const { return error == ErrorCode::Success; }
    bool hasValue() const { return value.has_value(); }
    
    operator bool() const { return isSuccess(); }
};

} // namespace hue4cpp
