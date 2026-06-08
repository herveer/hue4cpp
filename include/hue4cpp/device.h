#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <ReactiveLitepp/ObservableObject.h>

/**
 * @file device.h
 * @brief Physical device representation aggregating its lights and sensors
 */

namespace hue4cpp {
	using namespace ReactiveLitepp;

	// Forward declarations
	class Bridge;
	class Light;
	class Sensor;

	/**
	 * @brief Represents a physical Hue device (e.g. a bulb, a dimmer switch, a motion sensor)
	 *
	 * A device groups together the control/state services it exposes. In hue4cpp those
	 * services are surfaced as the Light and Sensor objects the device owns, alongside the
	 * product metadata reported by the bridge (model, archetype, software version, ...).
	 *
	 * The contained Light and Sensor objects are the same reactive objects returned by
	 * Bridge::getLights() / Bridge::getSensors(); subscribing to their properties gives
	 * real-time updates. The lists are always ordered to follow the device's declared
	 * service order, so repeated calls yield a stable ordering.
	 *
	 * @note Device, like Light and Sensor, is an ObservableObject and is therefore
	 *       non-copyable and non-movable. Obtain instances through Bridge::getDevices()
	 *       or Bridge::getDevice().
	 */
	class Device : public ObservableObject {
	public:
		/**
		 * @brief Default constructor
		 */
		Device();

		/**
		 * @brief Construct a Device with ID and parent bridge
		 * @param id Device unique identifier
		 * @param bridge Pointer to parent bridge
		 */
		Device(const std::string& id, Bridge* bridge);

		/**
		 * @brief Destructor
		 */
		~Device();

		// Prevent copying and moving (holds unique_ptrs to non-movable Light/Sensor objects)
		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&&) = delete;
		Device& operator=(Device&&) = delete;

		/**
		 * @brief The device's unique identifier (read-only)
		 */
		ReadonlyProperty<std::string> Id{
			[this]() { return _id; }
		};

		/**
		 * @brief The resource type of the device (always "device") (read-only)
		 */
		ReadonlyProperty<std::string> Type{
			[this]() { return _type; }
		};

		/**
		 * @brief Human readable name given to the device (read-only)
		 */
		ReadonlyProperty<std::string> Name{
			[this]() { return _name; }
		};

		/**
		 * @brief Unique identification of the device model (read-only)
		 */
		ReadonlyProperty<std::string> ModelId{
			[this]() { return _modelId; }
		};

		/**
		 * @brief Name of the device manufacturer (read-only)
		 */
		ReadonlyProperty<std::string> ManufacturerName{
			[this]() { return _manufacturerName; }
		};

		/**
		 * @brief Name of the product (read-only)
		 */
		ReadonlyProperty<std::string> ProductName{
			[this]() { return _productName; }
		};

		/**
		 * @brief Archetype of the product (read-only)
		 */
		ReadonlyProperty<std::string> ProductArchetype{
			[this]() { return _productArchetype; }
		};

		/**
		 * @brief Whether this device is Hue certified (read-only)
		 */
		ReadonlyProperty<bool> Certified{
			[this]() { return _certified; }
		};

		/**
		 * @brief Software version of the product (read-only)
		 */
		ReadonlyProperty<std::string> SoftwareVersion{
			[this]() { return _softwareVersion; }
		};

		/**
		 * @brief Get the lights owned by this device
		 * @return Const reference to the device's light list, ordered by the device's service order
		 */
		const std::vector<std::unique_ptr<Light>>& getLights() const;

		/**
		 * @brief Get the sensors owned by this device
		 * @return Const reference to the device's sensor list, ordered by the device's service order
		 */
		const std::vector<std::unique_ptr<Sensor>>& getSensors() const;

		/**
		 * @brief Initialize device metadata from JSON data
		 * @param json JSON object containing device data from the API
		 * @note This is an internal method used by Bridge to populate device data.
		 *       It does not populate the light/sensor lists; Bridge attaches those.
		 */
		void initFromJson(const nlohmann::json& json);

	private:
		std::string _id;
		std::string _type;
		std::string _name;
		std::string _modelId;
		std::string _manufacturerName;
		std::string _productName;
		std::string _productArchetype;
		bool _certified = false;
		std::string _softwareVersion;

		std::vector<std::unique_ptr<Light>> _lights;
		std::vector<std::unique_ptr<Sensor>> _sensors;

		Bridge* _bridge = nullptr;

		/**
		 * @brief Append a light to this device's light list
		 * @param light Light to take ownership of
		 */
		void addLight(std::unique_ptr<Light> light);

		/**
		 * @brief Append a sensor to this device's sensor list
		 * @param sensor Sensor to take ownership of
		 */
		void addSensor(std::unique_ptr<Sensor> sensor);

		friend class Bridge;
	};

} // namespace hue4cpp
