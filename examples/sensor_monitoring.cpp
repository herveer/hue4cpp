/**
 * @file sensor_monitoring.cpp
 * @brief Example demonstrating sensor access and real-time monitoring
 *
 * This example shows how to:
 * 1. Connect to a Hue bridge
 * 2. Discover and enumerate all sensors
 * 3. Read sensor states (motion, temperature, light level, buttons)
 * 4. Monitor sensor state changes in real-time via SSE
 */

#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <thread>
#include <signal.h>
#include <atomic>

using namespace hue4cpp;

// Global flag for graceful shutdown
std::atomic<bool> running(true);

void signalHandler(int signum) {
	std::cout << "\n\nInterrupt signal (" << signum << ") received. Stopping...\n";
	running = false;
}

std::string getCurrentTime() {
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
	ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
	return ss.str();
}

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
	default:
		return "Unknown";
	}
}

std::string buttonEventToString(ButtonEvent event) {
	switch (event) {
	case ButtonEvent::InitialPress:
		return "Initial Press";
	case ButtonEvent::ShortRelease:
		return "Short Release";
	case ButtonEvent::LongRelease:
		return "Long Release";
	case ButtonEvent::LongPress:
		return "Long Press";
	case ButtonEvent::DoubleShortRelease:
		return "Double Short Release";
	case ButtonEvent::Repeat:
		return "Repeat";
	default:
		return "Unknown";
	}
}

void printSensorInfo(const Sensor* sensor) {
	std::cout << "  ID: " << sensor->getId() << std::endl;
	std::cout << "  Type: " << sensorTypeToString(sensor->getType()) << std::endl;
	std::cout << "  Enabled: " << (sensor->isEnabled() ? "Yes" : "No") << std::endl;

	if (const auto* s = dynamic_cast<const MotionSensor*>(sensor)) {
		std::cout << "  Motion Detected: " << ((bool)s->Motion ? "YES" : "NO") << std::endl;
		std::cout << "  Valid: " << ((bool)s->MotionValid ? "Yes" : "No") << std::endl;
	}
	else if (const auto* s = dynamic_cast<const TemperatureSensor*>(sensor)) {
		std::cout << "  Temperature: " << std::fixed << std::setprecision(2)
			<< (float)s->Temperature << " °C" << std::endl;
		std::cout << "  Valid: " << ((bool)s->TemperatureValid ? "Yes" : "No") << std::endl;
	}
	else if (const auto* s = dynamic_cast<const LightLevelSensor*>(sensor)) {
		std::cout << "  Light Level: " << (uint32_t)s->LightLevel << std::endl;
		std::cout << "  Valid: " << ((bool)s->LightLevelValid ? "Yes" : "No") << std::endl;
	}
	else if (const auto* s = dynamic_cast<const ButtonSensor*>(sensor)) {
		std::cout << "  Last Event: " << buttonEventToString((ButtonEvent)s->LastEvent) << std::endl;
		std::cout << "  Button ID: " << (uint32_t)s->ButtonId << std::endl;
		std::cout << "  Event Sequence: " << (uint32_t)s->EventSequence << std::endl;
	}
}

void printEventInfo(const hue4cpp::Event& event) {
	std::cout << "[" << getCurrentTime() << "] ";

	switch (event.type) {
	case EventType::SensorStateChanged:
		std::cout << "Sensor state changed: " << event.resource_id << std::endl;
		if (!event.data.empty()) {
			// Pretty print the sensor state (simplified)
			std::cout << "               Data: " << nlohmann::json::parse(event.data).dump(2) << std::endl;
		}
		break;

	case EventType::SensorAdded:
		std::cout << "Sensor added: " << event.resource_id << std::endl;
		break;

	case EventType::SensorRemoved:
		std::cout << "Sensor removed: " << event.resource_id << std::endl;
		break;

	case EventType::LightStateChanged:
		std::cout << "Light state changed: " << event.resource_id << std::endl;
		break;

	case EventType::BridgeConnected:
		std::cout << "Bridge connected\n";
		break;

	case EventType::BridgeDisconnected:
		std::cout << "Bridge disconnected\n";
		break;

	case EventType::Unknown:
	default:
		std::cout << "Unknown event\n";
		break;
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

int main() {
	std::cout << "=== Hue4cpp Sensor Monitoring Example ===" << std::endl << std::endl;

	// Register signal handler for graceful shutdown
	signal(SIGINT, signalHandler);

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

		// Authenticate if needed
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

				auto auth_result = bridge.authenticate("hue4cpp-color-example", "computer");
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

		// Step 3: Get all sensors
		std::cout << "\n--- Discovering Sensors ---" << std::endl;
		auto sensors = bridge.getSensors();

		if (sensors.empty()) {
			std::cout << "No sensors found!" << std::endl;
		}
		else {
			std::cout << "Found " << sensors.size() << " sensor(s):\n" << std::endl;

			for (const auto& sensor : sensors) {
				printSensorInfo(sensor.get());
				std::cout << std::endl;
			}
		}

		// Step 4: Get sensors by type
		std::cout << "\n--- Sensors by Type ---" << std::endl;

		auto motion_sensors = bridge.getMotionSensors();
		std::cout << "Motion sensors: " << motion_sensors.size() << std::endl;

		auto temp_sensors = bridge.getTemperatureSensors();
		std::cout << "Temperature sensors: " << temp_sensors.size() << std::endl;

		auto light_sensors = bridge.getLightLevelSensors();
		std::cout << "Light level sensors: " << light_sensors.size() << std::endl;

		auto button_sensors = bridge.getButtonSensors();
		std::cout << "Button sensors: " << button_sensors.size() << std::endl;

		// Step 5: Start real-time monitoring
		std::cout << "\n--- Starting Real-Time Monitoring ---" << std::endl;
		std::cout << "Monitoring sensor state changes (Press Ctrl+C to stop)...\n" << std::endl;

		for (auto& button : button_sensors) {
			button->PropertyChanged += [](ObservableObject& _, const PropertyChangeArgs& event) {
				std::cout << "[" << getCurrentTime() << "] Button sensor property changed: " << event.PropertyName() << std::endl;
				};
		}
		auto& state_manager = bridge.getStateManager();

		// Subscribe to sensor events using ReactiveLitepp
		auto event_sub = state_manager.OnResourceEvent.SubscribeScoped(
			[](const ResourceEventArgs& e) {
				std::cout << "[" << getCurrentTime() << "] ";
				switch (e.type) {
				case EventType::SensorStateChanged:
					std::cout << "Sensor state changed: " << e.resource_id << "\n";
					if (!e.state_json.empty())
						std::cout << "               Data: "
						<< nlohmann::json::parse(e.state_json).dump(2) << "\n";
					break;
				case EventType::SensorAdded:
					std::cout << "Sensor added: " << e.resource_id << "\n";
					break;
				case EventType::SensorRemoved:
					std::cout << "Sensor removed: " << e.resource_id << "\n";
					break;
				case EventType::LightStateChanged:
					std::cout << "Light state changed: " << e.resource_id << "\n";
					break;
				case EventType::BridgeConnected:
					std::cout << "Bridge connected\n";
					break;
				case EventType::BridgeDisconnected:
					std::cout << "Bridge disconnected\n";
					break;
				default:
					break;
				}
			});

		std::cout << "Waiting for sensor events..." << std::endl << std::endl;

		// Keep monitoring until interrupted
		while (running && state_manager.isRunning()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Cleanup – scoped subscriptions auto-unsubscribe when they go out of scope
		std::cout << "\nStopping state monitoring..." << std::endl;
		state_manager.stop();

		std::cout << "Done!" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
