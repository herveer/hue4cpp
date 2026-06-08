#include "hue4cpp/device.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/sensors.h"
#include "hue4cpp/json_utils.h"

namespace hue4cpp {

	// Device implementation
	Device::Device() : _bridge(nullptr) {}

	Device::Device(const std::string& id, Bridge* bridge)
		: _id(id), _bridge(bridge) {
	}

	// Defined here (not defaulted in the header) so the unique_ptr destructors
	// of the contained Light/Sensor objects see complete types.
	Device::~Device() = default;

	const std::vector<std::unique_ptr<Light>>& Device::getLights() const {
		return _lights;
	}

	const std::vector<std::unique_ptr<Sensor>>& Device::getSensors() const {
		return _sensors;
	}

	void Device::addLight(std::unique_ptr<Light> light) {
		if (light) {
			_lights.push_back(std::move(light));
		}
	}

	void Device::addSensor(std::unique_ptr<Sensor> sensor) {
		if (sensor) {
			_sensors.push_back(std::move(sensor));
		}
	}

	void Device::initFromJson(const nlohmann::json& json) {
		auto idVal = json_utils::getValueOr<std::string>(json, "id", _id);
		SetPropertyValueAndNotify<&Device::Id>(_id, idVal);

		auto typeVal = json_utils::getValueOr<std::string>(json, "type", _type);
		SetPropertyValueAndNotify<&Device::Type>(_type, typeVal);

		// Human readable name lives under metadata.
		if (json.contains("metadata") && json["metadata"].is_object()) {
			auto nameVal = json_utils::getValueOr<std::string>(json["metadata"], "name", _name);
			SetPropertyValueAndNotify<&Device::Name>(_name, nameVal);
		}

		// Product information.
		if (json.contains("product_data") && json["product_data"].is_object()) {
			const auto& product = json["product_data"];

			SetPropertyValueAndNotify<&Device::ModelId>(
				_modelId, json_utils::getValueOr<std::string>(product, "model_id", _modelId));
			SetPropertyValueAndNotify<&Device::ManufacturerName>(
				_manufacturerName, json_utils::getValueOr<std::string>(product, "manufacturer_name", _manufacturerName));
			SetPropertyValueAndNotify<&Device::ProductName>(
				_productName, json_utils::getValueOr<std::string>(product, "product_name", _productName));
			SetPropertyValueAndNotify<&Device::ProductArchetype>(
				_productArchetype, json_utils::getValueOr<std::string>(product, "product_archetype", _productArchetype));
			SetPropertyValueAndNotify<&Device::Certified>(
				_certified, json_utils::getValueOr<bool>(product, "certified", _certified));
			SetPropertyValueAndNotify<&Device::SoftwareVersion>(
				_softwareVersion, json_utils::getValueOr<std::string>(product, "software_version", _softwareVersion));
		}
	}

} // namespace hue4cpp
