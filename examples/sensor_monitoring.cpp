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

void printSensorInfo(const Sensor& sensor) {
	std::cout << "  ID: " << sensor.getId() << std::endl;
	std::cout << "  Type: " << sensorTypeToString(sensor.getType()) << std::endl;
	std::cout << "  Enabled: " << (sensor.isEnabled() ? "Yes" : "No") << std::endl;

	// Print type-specific state
	if (sensor.getType() == SensorType::Motion) {
		auto state = sensor.getMotionState();
		if (state.has_value()) {
			std::cout << "  Motion Detected: " << (state->motion ? "YES" : "NO") << std::endl;
			std::cout << "  Valid: " << (state->motion_valid ? "Yes" : "No") << std::endl;
		}
	}
	else if (sensor.getType() == SensorType::Temperature) {
		auto state = sensor.getTemperatureState();
		if (state.has_value()) {
			std::cout << "  Temperature: " << std::fixed << std::setprecision(2)
				<< state->temperature << " °C" << std::endl;
			std::cout << "  Valid: " << (state->temperature_valid ? "Yes" : "No") << std::endl;
		}
	}
	else if (sensor.getType() == SensorType::LightLevel) {
		auto state = sensor.getLightLevelState();
		if (state.has_value()) {
			std::cout << "  Light Level: " << state->light_level << std::endl;
			std::cout << "  Valid: " << (state->light_level_valid ? "Yes" : "No") << std::endl;
		}
	}
	else if (sensor.getType() == SensorType::Button) {
		auto state = sensor.getButtonState();
		if (state.has_value()) {
			std::cout << "  Last Event: " << buttonEventToString(state->last_event) << std::endl;
			std::cout << "  Button ID: " << state->button_id << std::endl;
			std::cout << "  Event Sequence: " << state->event_sequence << std::endl;
		}
	}
}

void printEventInfo(const Event& event) {
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
bool loadConfig(std::string& bridge_ip, std::string& auth_key) {
	std::ifstream config_file("hue_config.txt");
	if (!config_file.is_open()) {
		return false;
	}

	std::getline(config_file, bridge_ip);
	std::getline(config_file, auth_key);
	config_file.close();

	return !bridge_ip.empty() && !auth_key.empty();
}

void saveConfig(const std::string& bridge_ip, const std::string& auth_key) {
	std::ofstream config_file("hue_config.txt");
	config_file << bridge_ip << std::endl;
	config_file << auth_key << std::endl;
	config_file.close();
}

int main() {
	std::cout << "=== Hue4cpp Sensor Monitoring Example ===" << std::endl << std::endl;

	// Register signal handler for graceful shutdown
	signal(SIGINT, signalHandler);

	try {
		// Step 1: Get bridge connection
		std::string bridge_ip, auth_key;
		if (!loadConfig(bridge_ip, auth_key)) {
			std::cout << "No saved configuration found. Discovering bridges..." << std::endl;

			auto bridges = Bridge::discover();
			if (bridges.empty()) {
				std::cout << "No bridges found!" << std::endl;
				return 1;
			}

			// Use the first discovered bridge
			auto& bridge_info = bridges[0].getInfo();
			bridge_ip = bridge_info.ip_address;

			std::cout << "Found bridge: " << bridge_info.name
				<< " (" << bridge_ip << ")" << std::endl;
			std::cout << "\nPress the link button on your bridge and then press Enter...";
			std::cin.get();

			// Authenticate
			auto result = bridges[0].authenticate("hue4cpp", "sensor_monitoring");
			if (!result || !result.hasValue()) {
				std::cout << "Authentication failed: " << result.error_message << std::endl;
				return 1;
			}

			auth_key = result.value.value();
			saveConfig(bridge_ip, auth_key);
			std::cout << "Authentication successful! Configuration saved." << std::endl;
		}
		else {
			std::cout << "Using saved configuration:" << std::endl;
			std::cout << "  Bridge IP: " << bridge_ip << std::endl;
		}

		// Step 2: Create bridge connection
		BridgeInfo info(bridge_ip, "");
		Bridge bridge(info);
		bridge.setAuthenticationKey(auth_key);

		// Validate authentication
		auto validation = bridge.validateAuthentication();
		if (!validation.isSuccess()) {
			std::cout << "Authentication failed. Please delete hue_config.txt and try again." << std::endl;
			return 1;
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
				printSensorInfo(sensor);
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

		auto& state_manager = bridge.getStateManager();

		// Register callback for all events
		auto callback_id = state_manager.registerCallback([](const Event& event) {
			printEventInfo(event);
			});

		// Start SSE connection
		auto start_result = state_manager.start();
		if (!start_result.isSuccess()) {
			std::cout << "Failed to start state monitoring: " << start_result.error_message << std::endl;
			return 1;
		}

		std::cout << "State monitoring started successfully!" << std::endl;
		std::cout << "Waiting for sensor events..." << std::endl << std::endl;

		// Keep monitoring until interrupted
		while (running && state_manager.isRunning()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Cleanup
		std::cout << "\nStopping state monitoring..." << std::endl;
		state_manager.unregisterCallback(callback_id);
		state_manager.stop();

		std::cout << "Done!" << std::endl;

	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
