/**
 * @file device_listing.cpp
 * @brief Example listing every device available on the bridge
 *
 * This example shows how to:
 * 1. Connect to a Hue bridge (reusing a saved authentication key when available)
 * 2. Enumerate all devices via Bridge::getDevices()
 * 3. Print each device's product metadata together with the lights and
 *    sensors it owns
 *
 * The authentication-key save/load mechanism mirrors the sensor_monitoring
 * example: the key is persisted to a small text file keyed by bridge id, so
 * the link-button flow only has to run once.
 */

#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace hue4cpp;

std::string sensorTypeToString(SensorType type) {
	switch (type) {
	case SensorType::Motion:
		return "Motion";
	case SensorType::Temperature:
		return "Temperature";
	case SensorType::LightLevel:
		return "Light Level";
	case SensorType::Button:
		return "Button";
	case SensorType::CameraMotion:
		return "Camera Motion";
	case SensorType::BellButton:
		return "Bell Button";
	case SensorType::RelativeRotary:
		return "Relative Rotary";
	case SensorType::Geolocation:
		return "Geolocation";
	case SensorType::Tamper:
		return "Tamper";
	case SensorType::Unknown:
	default:
		return "Unknown";
	}
}

// Helper functions to load/save configuration
const std::string KEY_FILE = "hue_auth_key.txt";

/**
 * @brief Save authentication key to a file
 */
bool saveAuthKey(const std::string& bridge_id, const std::string& key) {
	try {
		std::ofstream file(KEY_FILE);
		if (!file.is_open()) {
			return false;
		}
		file << bridge_id << std::endl;
		file << key << std::endl;
		file.close();
		return true;
	}
	catch (...) {
		return false;
	}
}

/**
 * @brief Load authentication key from a file
 */
std::string loadAuthKey(const std::string& bridge_id) {
	try {
		std::ifstream file(KEY_FILE);
		if (!file.is_open()) {
			return "";
		}

		std::string saved_bridge_id;
		std::string key;

		std::getline(file, saved_bridge_id);
		std::getline(file, key);
		file.close();

		// Check if the key is for the same bridge
		std::string lower_case_bridge_id = bridge_id;
		std::transform(lower_case_bridge_id.begin(), lower_case_bridge_id.end(),
			lower_case_bridge_id.begin(), ::tolower);

		if (saved_bridge_id == lower_case_bridge_id || saved_bridge_id == bridge_id) {
			return key;
		}

		return "";
	}
	catch (...) {
		return "";
	}
}

void printDeviceInfo(const Device* device) {
	std::cout << "  Name:         " << device->Name.Get() << std::endl;
	std::cout << "  Product:      " << device->ProductName.Get() << std::endl;
	std::cout << "  Manufacturer: " << device->ManufacturerName.Get() << std::endl;
	std::cout << "  Model ID:     " << device->ModelId.Get() << std::endl;
	std::cout << "  Archetype:    " << device->ProductArchetype.Get() << std::endl;
	std::cout << "  Software:     " << device->SoftwareVersion.Get() << std::endl;
	std::cout << "  Certified:    " << (device->Certified.Get() ? "Yes" : "No") << std::endl;
	std::cout << "  Type:         " << device->Type.Get() << std::endl;
	std::cout << "  ID:           " << device->Id.Get() << std::endl;

	// Lights owned by the device (ordered following the device's service order)
	const auto& lights = device->getLights();
	std::cout << "  Lights (" << lights.size() << "):" << std::endl;
	for (const auto& light : lights) {
		std::cout << "    - \"" << light->Name.Get() << "\" [" << light->Id.Get() << "]" << std::endl;
	}

	// Sensors owned by the device (same ordering guarantee)
	const auto& sensors = device->getSensors();
	std::cout << "  Sensors (" << sensors.size() << "):" << std::endl;
	for (const auto& sensor : sensors) {
		std::cout << "    - " << sensorTypeToString(sensor->getType())
			<< " \"" << sensor->Name.Get() << "\" [" << sensor->getId() << "]" << std::endl;
	}
}

int main() {
	std::cout << "=== Hue4cpp Device Listing Example ===" << std::endl << std::endl;

	try {
		// Step 1: Discover bridges
		std::cout << "Discovering bridges...\n";
		auto bridges = Bridge::discover();

		if (bridges.empty()) {
			std::cout << "❌ No bridges found. Please ensure your bridge is on the network.\n";
			return 1;
		}

		std::cout << "Found " << bridges.size() << " bridge(s)\n";
		auto& bridge = bridges[0];
		const auto& info = bridge.getInfo();
		std::cout << "   Bridge: " << info.name << " (" << info.ip_address << ")\n\n";

		// Step 2: Authenticate if needed
		if (!bridge.isAuthenticated()) {

			// Try to load saved authentication key
			std::cout << "\nChecking for saved authentication key..." << std::endl;
			std::string saved_key = loadAuthKey(info.id);
			if (!saved_key.empty()) {
				std::cout << "Found saved authentication key!" << std::endl;
				bridge.setAuthenticationKey(saved_key);
				std::cout << "Validating key with bridge..." << std::endl;
				auto validation_result = bridge.validateAuthentication();
				if (validation_result.isSuccess()) {
					std::cout << "Saved key is valid!" << std::endl;
				}
			}
			// If still not authenticated, perform authentication
			if (!bridge.isAuthenticated()) {
				std::cout << "\nPlease press the button on your Hue bridge..." << std::endl;
				std::cout << "Press Enter when ready...";
				std::cin.get();

				auto auth_result = bridge.authenticate("hue4cpp-device-example", "computer");
				if (!auth_result.isSuccess()) {
					std::cerr << "Authentication failed: " << auth_result.error_message << std::endl;
					return 1;
				}

				std::cout << "Authentication successful!" << std::endl;
				// Save the new key
				if (auth_result.hasValue()) {
					std::string auth_key = auth_result.value.value();
					if (saveAuthKey(info.id, auth_key)) {
						std::cout << "Authentication key saved." << std::endl;
					}
				}
			}

		}

		std::cout << "\n=== Connected to Bridge ===" << std::endl;

		// Step 3: List all devices
		std::cout << "\n--- Discovering Devices ---" << std::endl;
		auto devices = bridge.getDevices();

		if (devices.empty()) {
			std::cout << "No devices found!" << std::endl;
		}
		else {
			std::cout << "Found " << devices.size() << " device(s):\n" << std::endl;

			int index = 1;
			for (const auto& device : devices) {
				std::cout << "[" << index++ << "]" << std::endl;
				printDeviceInfo(device.get());
				std::cout << std::endl;
			}
		}

		std::cout << "Done!" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
