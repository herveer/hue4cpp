#pragma once

#include "types.h"
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include "hue4cpp/color_utils.h"
#include "hue4cpp/exceptions.h"
#include <ReactiveLitepp/ObservableObject.h>
#include <ReactiveLitepp/ScopedSubscription.h>
/**
 * @file light.h
 * @brief Light control and management
 */

namespace hue4cpp {
	using namespace ReactiveLitepp;

	// Forward declarations
	class Bridge;
	struct ResourceEventArgs; ///< From state.h - forward declared to avoid circular includes

	/**
	 * @brief Represents a single Hue light
	 *
	 * The Light class provides control of individual lights through observable properties
	 * including power state, brightness, color, and effects. All control operations are
	 * performed through the public properties, which handle exceptions appropriately.
	 *
	 * @throws ResourceNotFoundException if light data is not available
	 * @throws InvalidParameterException if invalid parameters are provided
	 * @throws BridgeNotReachableException if bridge cannot be accessed
	 */
	class Light : public ObservableObject {
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

		// Prevent copying and moving
		Light(const Light&) = delete;
		Light& operator=(const Light&) = delete;
		Light(Light&&) = delete;
		Light& operator=(Light&&) = delete;

		/**
		 * @brief Get the light's unique identifier
		 * @return Light ID
		 */
		ReadonlyProperty<std::string> Id{
			[this]() {
					return _id;
			}
		};

		/**
		 * @brief Get or set the light's display name
		 *
		 * Assigning a new value PUTs @c {"metadata":{"name":"…"}} to the bridge
		 * immediately. The bridge confirms the rename via an SSE event, which
		 * updates the backing field and fires @c PropertyChanged a second time.
		 * @throws InvalidParameterException if the name is empty
		 * @throws BridgeNotReachableException if the bridge is not available
		 * @throws AuthenticationException if not authenticated
		 */
		Property<std::string> Name{
			[this]() { return _name; },
			[this](std::string& value) {
				try {
					NotifyPropertyChanging<&Light::Name>();
					setName(value);
					NotifyPropertyChanged<&Light::Name>();
				}
				catch (const HueException&) { throw; }
			}
		};

		/**
		 * @brief Get the light's capabilities
		 * @return LightCapabilities structure
		 */
		LightCapabilities getCapabilities() const;

		/**
		 * @brief Refresh light state from bridge
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws ResourceNotFoundException if light cannot be found
		 * @throws AuthenticationException if not authenticated
		 */
		void refresh();

		/**
		 * @brief Initialize light state from JSON data
		 * @param json JSON object containing light data from API
		 * @note This is an internal method used by Bridge to populate light data
		 */
		void initFromJson(const nlohmann::json& json);

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
		 * @brief Get or set the on/off state
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws InvalidParameterException if light does not support on/off
		 * @throws AuthenticationException if not authenticated
		 * @throws ResourceNotFoundException if state is not available
		 */
		Property<bool> IsOn{
			[this]() {
				try {
					return isOn();
				}
				catch (const HueException&) {
				    throw;
				}
			},
			[this](bool& value) {
				try {
					NotifyPropertyChanging<&Light::IsOn>();
					value ? turnOn() : turnOff();
					// NotifyPropertyChanged call when the sse event is received
				}
				catch (const HueException&) {
				    throw;
				}
			}
		};

		/**
		 * @brief Get or set the brightness level
		 * @throws ResourceNotFoundException if brightness is not available
		 * @throws InvalidParameterException if brightness is out of range or not supported
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws AuthenticationException if not authenticated
		 */
		Property<uint8_t> Brightness{
			[this]() {
				try {
					return getBrightness();
				}
				catch (const HueException&) {
					throw;
				}
			},
			[this](uint8_t& value) {
				try {
					NotifyPropertyChanging<&Light::Brightness>();
					setBrightness(value);
					// NotifyPropertyChanged call when the sse event is received
				}
				catch (const HueException&) {
					throw;
				}
			}
		};

		/**
		 * @brief Get or set the color in XY color space
		 * @throws ResourceNotFoundException if color is not available
		 * @throws InvalidParameterException if color is invalid or not supported
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws AuthenticationException if not authenticated
		 */
		Property<XYColor> XYColor_{
			[this]() {
				try {
					return getColor();
				}
				catch (const HueException&) {
					throw;
				}
			},
			[this](XYColor& value) {
				try {
					NotifyPropertyChanging<&Light::XYColor_>();
					setColor(value);					
					// NotifyPropertyChanged call when the sse event is received
				}
				catch (const HueException&) {
					throw;
				}
			}
		};

		/**
		 * @brief Get or set the color in RGB color space
		 * @throws ResourceNotFoundException if color is not available
		 * @throws InvalidParameterException if color is invalid or not supported
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws AuthenticationException if not authenticated
		 */
		Property<RGBColor> RGBColor_{
			[this]() {
				try {
					return color_utils::xyToRgb(getColor());
				}
				catch (const HueException&) {
					throw;
				}
			},
			[this](RGBColor& value) {
				try {
					NotifyPropertyChanging<&Light::RGBColor_>();
					setColor(value);
					// NotifyPropertyChanged call when the sse event is received
				}
				catch (const HueException&) {
					throw;
				}
			}
		};

		/**
		 * @brief Get or set the color temperature
		 * @throws ResourceNotFoundException if color temperature is not available
		 * @throws InvalidParameterException if temperature is invalid or not supported
		 * @throws BridgeNotReachableException if bridge is not available
		 * @throws AuthenticationException if not authenticated
		 */
		Property<ColorTemperature> ColorTemperature_{
			[this]() {
				try {
					return getColorTemperature();
				}
				catch (const HueException&) {
					throw;
				}
			},
			[this](ColorTemperature& value) {
				try {
					NotifyPropertyChanging<&Light::ColorTemperature_>();
					setColorTemperature(value);
					// NotifyPropertyChanged call when the sse event is received
				}
				catch (const HueException&) {
					throw;
				}
			}
		};

	private:
		// Control methods - called from properties
		bool isOn() const;
		void turnOn() { turnOn(_transitionTime); }
		void turnOn(TransitionTime transition);
		void turnOff() { turnOff(_transitionTime); }
		void turnOff(TransitionTime transition);
		void toggle() { toggle(_transitionTime); }
		void toggle(TransitionTime transition);

		uint8_t getBrightness() const;
		void setBrightness(uint8_t brightness) { setBrightness(brightness, _transitionTime); }
		void setBrightness(uint8_t brightness, TransitionTime transition);

		XYColor getColor() const;
		void setColor(const XYColor& color) { setColor(color, _transitionTime); }
		void setColor(const XYColor& color, TransitionTime transition);
		void setColor(const RGBColor& color) { setColor(color, _transitionTime); }
		void setColor(const RGBColor& color, TransitionTime transition);
		void setColor(float r, float g, float b, TransitionTime transition);

		ColorTemperature getColorTemperature() const;
		void setColorTemperature(const ColorTemperature& temperature) { setColorTemperature(temperature, _transitionTime); }
		void setColorTemperature(const ColorTemperature& temperature, TransitionTime transition);

		void alert();
		void setName(const std::string& name);

		void sendUpdate(const nlohmann::json& state_update);
		void addTransitionTime(nlohmann::json& update, TransitionTime transition);
		TransitionTime _transitionTime = std::chrono::milliseconds(400);

		std::string _id;
		std::string _name;
		Bridge* _bridge;
		LightCapabilities _capabilities;

		/**
		 * @brief Subscribe to bridge StateManager::OnResourceEvent and set up property notifications.
		 * Called from both constructors when a bridge is available.
		 */
		void subscribeToBridgeEvents();

		/**
		 * @brief Handle a StateManager resource event.
		 * Fires ObservableObject property-change notifications for all affected properties.
		 * @param e The unified resource event args
		 */
		void onResourceEvent(const ResourceEventArgs& e);

		/// Scoped subscription to StateManager::OnResourceEvent.
		/// Automatically unsubscribes when this Light is destroyed.
		ScopedSubscription _bridgeEventSubscription;

		friend class Bridge;
	};

} // namespace hue4cpp
