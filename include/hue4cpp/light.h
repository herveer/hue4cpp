#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include "hue4cpp/color_utils.h"
#include <ReactiveLitepp/ObservableObject.h>
/**
 * @file light.h
 * @brief Light control and management
 */

namespace hue4cpp {
	using namespace ReactiveLitepp;
	// Forward declaration
	class Bridge;

	/**
	 * @brief Represents a single Hue light
	 *
	 * The Light class provides methods to control individual lights including
	 * power state, brightness, color, and effects.
	 */
	class Light : ObservableObject {
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
		~Light() = default;

		// Allow copying and moving
		Light(const Light&) = default;
		Light& operator=(const Light&) = default;
		Light(Light&&) noexcept = default;
		Light& operator=(Light&&) noexcept = default;

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
		Property<bool> IsOn{
			[this]() { return isOn(); },
			[this](bool& value) {
				NotifyPropertyChanging<&Light::IsOn>();
				value ? turnOn() : turnOff();
				NotifyPropertyChanged<&Light::IsOn>();
			}
		};

		/**
		* @brief Get or set the transition time for light changes
		* @return Transition time in milliseconds
		 * @note This property does not affect the current state but will be used for subsequent operations
		*/
		Property<TransitionTime> TransitionTime_
		{
			[this]() { return _transitionTime; },
			[this](TransitionTime& value) {
				SetPropertyValueAndNotify<&Light::TransitionTime_>(_transitionTime, value);
			}
		};

		/**
		 * @brief Toggle the light on/off
		 * @param transition Transition time (default: 400ms)
		 * @return Result indicating success or failure
		 */
		Result<void> toggle() { return toggle(_transitionTime); }
		Result<void> toggle(TransitionTime transition);

		/**
		 * @brief Get current brightness level
		 * @return Brightness (0-100), or nullopt if not available
		 */
		Property<std::optional<uint8_t>> Brightness{
			[this]() { return getBrightness(); },
			[this](std::optional<uint8_t>& value) {
				NotifyPropertyChanging<&Light::Brightness>();
				if (value) {
					setBrightness(*value);
				}
				NotifyPropertyChanged<&Light::Brightness>();
			}
		};

		/**
		 * @brief Get current color in XY color space
		 * @return XYColor, or nullopt if not available
		 */
		Property<std::optional<XYColor>> XYColor_{
			[this]() { return getColor(); },
			[this](std::optional<XYColor>& value) {
				NotifyPropertyChanging<&Light::XYColor_>();
				if (value) {
					setColor(*value);
				}
				NotifyPropertyChanged<&Light::XYColor_>();
			}
		};

		Property<std::optional<RGBColor>> RGBColor_{
			[this]() { return color_utils::xyToRgb(getColor().value()); },
			[this](std::optional<RGBColor>& value) {
				NotifyPropertyChanging<&Light::RGBColor_>();
				if (value) {
					setColor(*value);
				}
				NotifyPropertyChanged<&Light::RGBColor_>();
			}
		};

		/**
		 * @brief Get current color temperature
		 * @return ColorTemperature, or nullopt if not available
		 */
		Property<std::optional<ColorTemperature>> ColorTemperature_{
			[this]() { return getColorTemperature(); },
			[this](std::optional<ColorTemperature>& value) {
				NotifyPropertyChanging<&Light::ColorTemperature_>();
				if (value) {
					setColorTemperature(*value);
				}
				NotifyPropertyChanged<&Light::ColorTemperature_>();
			}
		};

		/**
		 * @brief Trigger alert effect (light blinks)
		 * @return Result indicating success or failure
		 */
		Result<void> alert();

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
		void initFromJson(const nlohmann::json& json);

	private:
		Result<void> sendUpdate(const nlohmann::json& state_update);
		void addTransitionTime(nlohmann::json& update, TransitionTime transition);
		TransitionTime _transitionTime = std::chrono::milliseconds(400);

		bool isOn() const;
		Result<void> turnOn() { return turnOn(_transitionTime); }
		Result<void> turnOn(TransitionTime transition);
		Result<void> turnOff() { return turnOff(_transitionTime); }
		Result<void> turnOff(TransitionTime transition);

		std::optional<uint8_t> getBrightness() const;
		Result<void> setBrightness(uint8_t brightness) { return setBrightness(brightness, _transitionTime); }
		Result<void> setBrightness(uint8_t brightness, TransitionTime transition);

		std::optional<XYColor> getColor() const;
		Result<void> setColor(const XYColor& color) { return setColor(color, _transitionTime); }
		Result<void> setColor(const XYColor& color, TransitionTime transition);
		Result<void> setColor(const RGBColor& color) { return setColor(color, _transitionTime); }
		Result<void> setColor(const RGBColor& color, TransitionTime transition);

		std::optional<ColorTemperature> getColorTemperature() const;
		Result<void> setColorTemperature(const ColorTemperature& temperature) { return setColorTemperature(temperature, _transitionTime); }
		Result<void> setColorTemperature(const ColorTemperature& temperature, TransitionTime transition);

		std::string id;
		std::string name;
		Bridge* bridge;
		LightCapabilities capabilities;

		friend class Bridge;
	};

} // namespace hue4cpp
