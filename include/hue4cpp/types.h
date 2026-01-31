#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <stdexcept>

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
			: r(red), g(green), b(blue) {
		}
	};

	/**
	 * @brief XY color representation (CIE 1931 color space)
	 */
	struct XYColor {
		float x; ///< X coordinate (0.0 - 1.0)
		float y; ///< Y coordinate (0.0 - 1.0)

		XYColor(float x_coord = 0.0f, float y_coord = 0.0f)
			: x(x_coord), y(y_coord) {
		}
	};

	/**
	 * @brief Color temperature in mireds
	 */
	struct ColorTemperature {
		uint16_t mireds; ///< Color temperature in mireds (153-500)

		explicit ColorTemperature(uint16_t value = 153)
			: mireds(value) {
		}

		/**
		 * @brief Create from Kelvin temperature
		 * @param kelvin Temperature in Kelvin (2000-6500)
		 * @return ColorTemperature in mireds
		 * @throws std::invalid_argument if kelvin is out of range
		 */
		static ColorTemperature fromKelvin(uint16_t kelvin) {
			if (kelvin < 2000 || kelvin > 6500) {
				throw std::invalid_argument("Kelvin temperature must be between 2000 and 6500");
			}
			return ColorTemperature(static_cast<uint16_t>(1000000 / kelvin));
		}

		/**
		 * @brief Convert to Kelvin temperature
		 * @return Temperature in Kelvin
		 * @throws std::invalid_argument if mireds is invalid
		 */
		uint16_t toKelvin() const {
			if (mireds == 0) {
				throw std::invalid_argument("Mireds value cannot be zero");
			}
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
	 *
	 * The Hue API supports transition times from 0ms to approximately 6553500ms.
	 * Default transition time is typically 400ms.
	 * A value of 0ms causes an instant change.
	 */
	using TransitionTime = std::chrono::milliseconds;

	/**
	 * @brief Sensor types supported by the Hue API
	 */
	enum class SensorType {
		Unknown,
		Motion,
		Temperature,
		LightLevel,
		Button,
		CameraMotion,
		BellButton,
		RelativeRotary,
		Geolocation,
		Tamper
	};

	/**
	 * @brief Motion sensor state
	 */
	struct MotionState {
		bool motion;       ///< Motion detected
		bool motion_valid; ///< Motion data is valid

		MotionState() : motion(false), motion_valid(false) {}
	};

	/**
	 * @brief Camera motion sensor state
	 */
	struct CameraMotionState {
		bool motion;       ///< Motion detected by camera
		bool motion_valid; ///< Motion data is valid

		CameraMotionState() : motion(false), motion_valid(false) {}
	};

	/**
	 * @brief Temperature sensor reading
	 */
	struct TemperatureState {
		float temperature; ///< Temperature in degrees Celsius
		bool temperature_valid; ///< Temperature data is valid

		TemperatureState() : temperature(0.0f), temperature_valid(false) {}
	};

	/**
	 * @brief Light level sensor reading
	 */
	struct LightLevelState {
		uint32_t light_level; ///< Illuminance in raw units (logarithmic scale)
		bool light_level_valid; ///< Light level data is valid

		LightLevelState() : light_level(0), light_level_valid(false) {}
	};

	/**
	 * @brief Button event types
	 */
	enum class ButtonEvent {
		Unknown,
		InitialPress,
		ShortRelease,
		LongRelease,
		LongPress,
		DoubleShortRelease,
		Repeat
	};

	/**
	 * @brief Button sensor state
	 */
	struct ButtonState {
		ButtonEvent last_event; ///< Last button event
		uint32_t button_id;     ///< Button ID (for multi-button devices)
		uint32_t event_sequence; ///< Event sequence number

		ButtonState() : last_event(ButtonEvent::Unknown), button_id(0), event_sequence(0) {}
	};

	/**
	 * @brief Bell button sensor state
	 */
	struct BellButtonState {
		ButtonEvent last_event; ///< Last button event (doorbell press)
		uint32_t event_sequence; ///< Event sequence number

		BellButtonState() : last_event(ButtonEvent::Unknown), event_sequence(0) {}
	};

	/**
	 * @brief Rotation direction for rotary sensors
	 */
	enum class RotationDirection {
		Unknown,
		ClockWise,
		CounterClockWise
	};

	/**
	 * @brief Relative rotary sensor state
	 */
	struct RelativeRotaryState {
		int32_t steps;                    ///< Number of rotation steps
		RotationDirection direction;      ///< Direction of rotation
		ButtonEvent action;               ///< Associated button action
		uint32_t event_sequence;          ///< Event sequence number

		RelativeRotaryState() 
			: steps(0), direction(RotationDirection::Unknown), 
			  action(ButtonEvent::Unknown), event_sequence(0) {}
	};

	/**
	 * @brief Geolocation sensor state
	 */
	struct GeolocationState {
		bool is_configured; ///< Whether geofencing is configured

		GeolocationState() : is_configured(false) {}
	};

	/**
	 * @brief Tamper detection sensor state
	 */
	struct TamperState {
		bool tampered;       ///< Tamper detected
		bool tamper_valid;   ///< Tamper data is valid

		TamperState() : tampered(false), tamper_valid(false) {}
	};

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
			: ip_address(ip), id(bridge_id) {
		}
		BridgeInfo(const BridgeInfo& other) = default;
		BridgeInfo(BridgeInfo&& other) noexcept
			: ip_address(std::move(other.ip_address)),
			id(std::move(other.id)),
			name(std::move(other.name)),
			model_id(std::move(other.model_id)),
			sw_version(std::move(other.sw_version)) {
		}

		BridgeInfo& operator=(const BridgeInfo& other) = default;
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
			: error(err), error_message(msg) {
		}

		bool isSuccess() const { return error == ErrorCode::Success; }
		bool hasValue() const { return value.has_value(); }

		operator bool() const { return isSuccess(); }
	};

	/**
	 * @brief Specialization for void Result
	 */
	template<>
	struct Result<void> {
		ErrorCode error;
		std::string error_message;

		Result() : error(ErrorCode::Success) {}
		Result(ErrorCode err, const std::string& msg = "")
			: error(err), error_message(msg) {
		}

		bool isSuccess() const { return error == ErrorCode::Success; }

		operator bool() const { return isSuccess(); }
	};

} // namespace hue4cpp
