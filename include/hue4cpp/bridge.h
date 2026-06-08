#pragma once

#include "types.h"
#include "state.h"
#include <vector>
#include <string>
#include <nlohmann/json_fwd.hpp>
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file bridge.h
 * @brief Bridge discovery and connection management
 */

namespace hue4cpp {

	// Forward declarations
	class Device;
	class Light;
	class Sensor;
	class MotionSensor;
	class TemperatureSensor;
	class LightLevelSensor;
	class ButtonSensor;
	class CameraMotionSensor;
	class BellButtonSensor;
	class RelativeRotarySensor;
	class GeolocationSensor;
	class TamperSensor;

	/**
	 * @brief Represents a Philips Hue Bridge
	 *
	 * The Bridge class is the main entry point for interacting with a Hue system.
	 * It handles discovery, authentication, and provides access to lights and other resources.
	 */
	class Bridge {
	public:
		/**
		 * @brief Default constructor
		 */
		Bridge();

		/**
		 * @brief Construct a Bridge with known connection info
		 * @param info Bridge connection information
		 */
		explicit Bridge(const BridgeInfo& info);

		/**
		 * @brief Destructor
		 */
		~Bridge();

		// Prevent copying, allow moving
		Bridge(const Bridge&) = delete;
		Bridge& operator=(const Bridge&) = delete;
		Bridge(Bridge&&) noexcept;
		Bridge& operator=(Bridge&&) noexcept;

		/**
		 * @brief Discover Hue bridges on the local network
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discover();

		/**
		 * @brief Discover bridges using mDNS
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discoverMDNS();

		/**
		 * @brief Discover bridges using N-UPnP (broker-based)
		 * @return Vector of discovered bridges
		 */
		static std::vector<Bridge> discoverNUPnP();

		/**
		 * @brief Authenticate with the bridge
		 * @param app_name Name of the application
		 * @param device_name Name of the device (optional)
		 * @return Result indicating success or failure
		 */
		Result<std::string> authenticate(const std::string& app_name,
			const std::string& device_name = "");

		/**
		 * @brief Set authentication key for the bridge
		 * @param key Previously obtained authentication key
		 */
		void setAuthenticationKey(const std::string& key);

		/**
		 * @brief Get the current authentication key
		 * @return The authentication key, or empty string if not authenticated
		 */
		std::string getAuthenticationKey() const;

		/**
		 * @brief Check if bridge is authenticated
		 * @return true if authenticated, false otherwise
		 */
		bool isAuthenticated() const;

		/**
		 * @brief Validate the current authentication key with the bridge
		 * @return Result indicating if the key is valid
		 */
		Result<void> validateAuthentication() const;

		/**
		 * @brief Get all lights connected to this bridge
		 * @return Vector of Light objects
		 */
		std::vector<std::unique_ptr<Light>> getLights();

		/**
		 * @brief Get a specific light by ID
		 * @param light_id The unique identifier of the light
		 * @return Unique pointer to Light object (nullptr if not found)
		 */
		std::unique_ptr<Light> getLight(const std::string& light_id);

		/**
		 * @brief Get all sensors connected to this bridge
		 * @return Vector of unique pointers to Sensor objects
		 */
		std::vector<std::unique_ptr<Sensor>> getSensors();

		/**
		 * @brief Get a specific sensor by ID
		 * @param sensor_id The unique identifier of the sensor
		 * @return Unique pointer to Sensor object (nullptr if not found)
		 */
		std::unique_ptr<Sensor> getSensor(const std::string& sensor_id);

		/**
		 * @brief Get all devices connected to this bridge
		 *
		 * Each device aggregates the Light and Sensor objects it owns. Those lists
		 * follow the device's declared service order, so the ordering is stable
		 * across calls.
		 * @return Vector of unique pointers to Device objects
		 */
		std::vector<std::unique_ptr<Device>> getDevices();

		/**
		 * @brief Get a specific device by ID
		 * @param device_id The unique identifier of the device
		 * @return Unique pointer to Device object (nullptr if not found)
		 */
		std::unique_ptr<Device> getDevice(const std::string& device_id);

		/**
		 * @brief Get all motion sensors
		 * @return Vector of unique pointers to MotionSensor objects
		 */
		std::vector<std::unique_ptr<MotionSensor>> getMotionSensors();

		/**
		 * @brief Get all temperature sensors
		 * @return Vector of unique pointers to TemperatureSensor objects
		 */
		std::vector<std::unique_ptr<TemperatureSensor>> getTemperatureSensors();

		/**
		 * @brief Get all light level sensors
		 * @return Vector of unique pointers to LightLevelSensor objects
		 */
		std::vector<std::unique_ptr<LightLevelSensor>> getLightLevelSensors();

		/**
		 * @brief Get all button sensors
		 * @return Vector of unique pointers to ButtonSensor objects
		 */
		std::vector<std::unique_ptr<ButtonSensor>> getButtonSensors();

		/**
		 * @brief Get all camera motion sensors
		 * @return Vector of unique pointers to CameraMotionSensor objects
		 */
		std::vector<std::unique_ptr<CameraMotionSensor>> getCameraMotionSensors();

		/**
		 * @brief Get all bell button sensors (doorbells)
		 * @return Vector of unique pointers to BellButtonSensor objects
		 */
		std::vector<std::unique_ptr<BellButtonSensor>> getBellButtonSensors();

		/**
		 * @brief Get all relative rotary sensors (dials/knobs)
		 * @return Vector of unique pointers to RelativeRotarySensor objects
		 */
		std::vector<std::unique_ptr<RelativeRotarySensor>> getRelativeRotarySensors();

		/**
		 * @brief Get all geolocation sensors
		 * @return Vector of unique pointers to GeolocationSensor objects
		 */
		std::vector<std::unique_ptr<GeolocationSensor>> getGeolocationSensors();

		/**
		 * @brief Get all tamper detection sensors
		 * @return Vector of unique pointers to TamperSensor objects
		 */
		std::vector<std::unique_ptr<TamperSensor>> getTamperSensors();

		/**
		 * @brief Set the bridge information
		 * @param info BridgeInfo structure
		 */
		void setInfo(const BridgeInfo& info);

		/**
		 * @brief Get bridge information
		 * @return BridgeInfo structure
		 */
		const BridgeInfo& getInfo() const;

		/**
		 * @brief Get the state manager for this bridge
		 * @return Reference to StateManager
		 */
		StateManager& getStateManager();

		/**
		 * @brief Get light state (cache-first, API-fallback)
		 * @param light_id The unique identifier of the light
		 * @param refreshCache If true, forces fetching the complete state from the bridge API
		 * @return JSON string with light state, or empty if not found
		 */
		std::string getLightState(const std::string& light_id, bool refreshCache);

		/**
		 * @brief Get sensor state (cache-first, API-fallback)
		 * @param sensor_id The unique identifier of the sensor
		 * @param sensor_type The type of sensor (motion, temperature, light_level, button)
		 * @param refreshCache If true, forces fetching the complete state from the bridge API
		 * @return JSON string with sensor state, or empty if not found
		 */
		std::string getSensorState(const std::string& sensor_id, const std::string& sensor_type, bool refreshCache);

		/**
		 * @brief Check if the bridge is reachable
		 * @return true if reachable, false otherwise
		 */
		bool isReachable() const;

	private:
		BridgeInfo _info;
		std::string _auth_key;
		std::unique_ptr<StateManager> _state_manager;

		template<typename SensorT>
		std::vector<std::unique_ptr<SensorT>> fetchSensorsByType(const std::string& resource_type);

		/**
		 * @brief Build Device objects from the raw "data" array of a device resource response.
		 *
		 * Fetches all lights and sensors once, then distributes them into each device by
		 * walking the device's services array in order so the resulting lists are stable.
		 * @param device_data JSON array of DeviceGet objects
		 * @return Vector of populated Device objects
		 */
		std::vector<std::unique_ptr<Device>> buildDevicesFromData(const nlohmann::json& device_data);
	};

} // namespace hue4cpp
