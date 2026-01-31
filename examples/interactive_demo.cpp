/**
 * @file interactive_demo.cpp
 * @brief Comprehensive interactive demo application for hue4cpp
 *
 * This demo provides a complete console-based interface for exploring
 * all features of the hue4cpp library. It serves as both a practical
 * tool and a learning resource.
 *
 * Features:
 * - Bridge discovery and connection
 * - Credential save/load functionality
 * - Light discovery and control
 * - Sensor discovery and monitoring
 * - Real-time event streaming (SSE)
 * - Clean page-based navigation
 */

#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <thread>
#include <limits>
#include <algorithm>
#include <cstdlib>

using namespace hue4cpp;

// ============================================================================
// Configuration and Helpers
// ============================================================================

const std::string CONFIG_FILE = "hue4cpp_demo_config.txt";
const int SCREEN_WIDTH = 80;

/**
 * @brief Clear the console screen (cross-platform)
 */
void clearScreen() {
#ifdef _WIN32
    int ret = system("cls");
    (void)ret;
#else
    int ret = system("clear");
    (void)ret;
#endif
}

/**
 * @brief Print a horizontal line
 */
void printLine(char c = '=') {
    std::cout << std::string(SCREEN_WIDTH, c) << std::endl;
}

/**
 * @brief Print a header with text centered
 */
void printHeader(const std::string& text) {
    clearScreen();
    printLine('=');
    int padding = (SCREEN_WIDTH - text.length()) / 2;
    std::cout << std::string(padding, ' ') << text << std::endl;
    printLine('=');
    std::cout << std::endl;
}

/**
 * @brief Wait for user to press Enter
 */
void waitForEnter() {
    std::cout << std::endl << "Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

/**
 * @brief Get current time as formatted string
 */
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

// ============================================================================
// Credential Management
// ============================================================================

/**
 * @brief Configuration data structure
 */
struct Config {
    std::string bridge_ip;
    std::string bridge_id;
    std::string auth_key;
};

/**
 * @brief Save configuration to file
 */
bool saveConfig(const Config& config) {
    try {
        std::ofstream file(CONFIG_FILE);
        if (!file.is_open()) {
            return false;
        }
        file << config.bridge_ip << std::endl;
        file << config.bridge_id << std::endl;
        file << config.auth_key << std::endl;
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief Load configuration from file
 */
bool loadConfig(Config& config) {
    try {
        std::ifstream file(CONFIG_FILE);
        if (!file.is_open()) {
            return false;
        }
        
        std::getline(file, config.bridge_ip);
        std::getline(file, config.bridge_id);
        std::getline(file, config.auth_key);
        file.close();
        
        return !config.bridge_ip.empty() && 
               !config.bridge_id.empty() && 
               !config.auth_key.empty();
    } catch (...) {
        return false;
    }
}

// ============================================================================
// String Formatting Helpers
// ============================================================================

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
        return "Rotary/Dial";
    case SensorType::Geolocation:
        return "Geolocation";
    case SensorType::Tamper:
        return "Tamper Detection";
    default:
        return "Unknown";
    }
}

std::string eventTypeToString(EventType type) {
    switch (type) {
    case EventType::LightStateChanged:
        return "Light State Changed";
    case EventType::LightAdded:
        return "Light Added";
    case EventType::LightRemoved:
        return "Light Removed";
    case EventType::SensorStateChanged:
        return "Sensor State Changed";
    case EventType::SensorAdded:
        return "Sensor Added";
    case EventType::SensorRemoved:
        return "Sensor Removed";
    case EventType::BridgeConnected:
        return "Bridge Connected";
    case EventType::BridgeDisconnected:
        return "Bridge Disconnected";
    default:
        return "Unknown Event";
    }
}

// ============================================================================
// Application State
// ============================================================================

struct AppState {
    std::unique_ptr<Bridge> bridge;
    Config config;
    bool connected = false;
    bool monitoring = false;
    uint64_t event_callback_id = 0;
};

// ============================================================================
// Page: Main Menu
// ============================================================================

int showMainMenu(AppState& state) {
    printHeader("HUE4CPP - Interactive Demo");
    
    std::cout << "Library Version: " << Version::STRING << std::endl;
    std::cout << std::endl;
    
    if (state.connected && state.bridge) {
        const auto& info = state.bridge->getInfo();
        std::cout << "Connected to: " << info.name << " (" << info.ip_address << ")" << std::endl;
        std::cout << std::endl;
    } else {
        std::cout << "Not connected to a bridge" << std::endl;
        std::cout << std::endl;
    }
    
    printLine('-');
    std::cout << "Main Menu:" << std::endl;
    std::cout << std::endl;
    std::cout << "  1. Connect to Bridge" << std::endl;
    std::cout << "  2. List All Lights" << std::endl;
    std::cout << "  3. Control a Light" << std::endl;
    std::cout << "  4. List All Sensors" << std::endl;
    std::cout << "  5. View Sensor Details" << std::endl;
    std::cout << "  6. Live Activity Monitor" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    printLine('-');
    
    std::cout << std::endl << "Choose an option: ";
    int choice;
    std::cin >> choice;
    
    return choice;
}

// ============================================================================
// Page: Connect to Bridge
// ============================================================================

void showConnectPage(AppState& state) {
    printHeader("Connect to Bridge");
    
    std::cout << "1. Load saved connection" << std::endl;
    std::cout << "2. Discover new bridge" << std::endl;
    std::cout << "0. Back to main menu" << std::endl;
    std::cout << std::endl << "Choose: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice == 0) {
        return;
    }
    
    if (choice == 1) {
        // Try to load saved configuration
        if (loadConfig(state.config)) {
            std::cout << std::endl << "Found saved configuration:" << std::endl;
            std::cout << "  Bridge IP: " << state.config.bridge_ip << std::endl;
            std::cout << "  Bridge ID: " << state.config.bridge_id << std::endl;
            std::cout << std::endl << "Connecting..." << std::endl;
            
            // Create bridge with saved info
            BridgeInfo info;
            info.ip_address = state.config.bridge_ip;
            info.id = state.config.bridge_id;
            
            state.bridge = std::make_unique<Bridge>(info);
            state.bridge->setAuthenticationKey(state.config.auth_key);
            
            // Validate authentication
            auto result = state.bridge->validateAuthentication();
            if (result.isSuccess()) {
                std::cout << "✓ Successfully connected!" << std::endl;
                state.connected = true;
            } else {
                std::cout << "✗ Authentication failed: " << result.error_message << std::endl;
                std::cout << "  Please connect using discovery." << std::endl;
                state.bridge.reset();
                state.connected = false;
            }
        } else {
            std::cout << std::endl << "No saved configuration found." << std::endl;
        }
        
        waitForEnter();
        return;
    }
    
    if (choice == 2) {
        // Discover bridges
        std::cout << std::endl << "Discovering bridges..." << std::endl;
        
        try {
            auto bridges = Bridge::discover();
            
            if (bridges.empty()) {
                std::cout << "✗ No bridges found on the network." << std::endl;
                std::cout << "  Make sure your bridge is connected to the same network." << std::endl;
                waitForEnter();
                return;
            }
            
            std::cout << std::endl << "Found " << bridges.size() << " bridge(s):" << std::endl;
            for (size_t i = 0; i < bridges.size(); i++) {
                const auto& info = bridges[i].getInfo();
                std::cout << "  " << (i + 1) << ". " << info.name 
                          << " (" << info.ip_address << ")" << std::endl;
            }
            
            std::cout << std::endl << "Select bridge (1-" << bridges.size() << "): ";
            size_t selection;
            std::cin >> selection;
            
            if (selection < 1 || selection > bridges.size()) {
                std::cout << "Invalid selection." << std::endl;
                waitForEnter();
                return;
            }
            
            auto& selected_bridge = bridges[selection - 1];
            const auto& info = selected_bridge.getInfo();
            
            // Authenticate
            std::cout << std::endl << "Authentication required." << std::endl;
            std::cout << "Please press the LINK BUTTON on your bridge now..." << std::endl;
            waitForEnter();
            
            std::cout << "Authenticating..." << std::endl;
            auto auth_result = selected_bridge.authenticate("hue4cpp-demo", "interactive-demo");
            
            if (!auth_result.isSuccess()) {
                std::cout << "✗ Authentication failed: " << auth_result.error_message << std::endl;
                waitForEnter();
                return;
            }
            
            std::cout << "✓ Authentication successful!" << std::endl;
            
            // Save configuration
            state.config.bridge_ip = info.ip_address;
            state.config.bridge_id = info.id;
            state.config.auth_key = auth_result.value.value();
            
            if (saveConfig(state.config)) {
                std::cout << "✓ Configuration saved to: " << CONFIG_FILE << std::endl;
            } else {
                std::cout << "⚠ Warning: Could not save configuration" << std::endl;
            }
            
            // Move bridge to app state
            state.bridge = std::make_unique<Bridge>(std::move(selected_bridge));
            state.connected = true;
            
        } catch (const std::exception& e) {
            std::cout << "✗ Error: " << e.what() << std::endl;
        }
        
        waitForEnter();
    }
}

// ============================================================================
// Page: List All Lights
// ============================================================================

void showLightsPage(AppState& state) {
    printHeader("All Lights");
    
    if (!state.connected || !state.bridge) {
        std::cout << "Not connected to a bridge." << std::endl;
        waitForEnter();
        return;
    }
    
    try {
        std::cout << "Fetching lights..." << std::endl;
        auto lights = state.bridge->getLights();
        
        if (lights.empty()) {
            std::cout << std::endl << "No lights found." << std::endl;
        } else {
            std::cout << std::endl << "Found " << lights.size() << " light(s):" << std::endl;
            std::cout << std::endl;
            
            for (size_t i = 0; i < lights.size(); i++) {
                const auto& light = lights[i];
                auto caps = light.getCapabilities();
                
                std::cout << (i + 1) << ". " << light.getName() << std::endl;
                std::cout << "   ID: " << light.getId() << std::endl;
                std::cout << "   State: " << (light.isOn() ? "ON" : "OFF") << std::endl;
                
                if (caps.brightness) {
                    auto brightness = light.getBrightness();
                    if (brightness.has_value()) {
                        std::cout << "   Brightness: " << static_cast<int>(brightness.value()) << "%" << std::endl;
                    }
                }
                
                std::cout << "   Capabilities: ";
                std::vector<std::string> cap_list;
                if (caps.on_off) cap_list.push_back("On/Off");
                if (caps.brightness) cap_list.push_back("Brightness");
                if (caps.color) cap_list.push_back("Color");
                if (caps.color_temperature) cap_list.push_back("Color Temp");
                if (caps.effects) cap_list.push_back("Effects");
                
                for (size_t j = 0; j < cap_list.size(); j++) {
                    std::cout << cap_list[j];
                    if (j < cap_list.size() - 1) std::cout << ", ";
                }
                std::cout << std::endl << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    waitForEnter();
}

// ============================================================================
// Page: Control a Light
// ============================================================================

void showLightControlPage(AppState& state) {
    printHeader("Light Control");
    
    if (!state.connected || !state.bridge) {
        std::cout << "Not connected to a bridge." << std::endl;
        waitForEnter();
        return;
    }
    
    try {
        auto lights = state.bridge->getLights();
        
        if (lights.empty()) {
            std::cout << "No lights found." << std::endl;
            waitForEnter();
            return;
        }
        
        std::cout << "Select a light:" << std::endl;
        for (size_t i = 0; i < lights.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << lights[i].getName() << std::endl;
        }
        std::cout << "  0. Back" << std::endl;
        std::cout << std::endl << "Choose: ";
        
        size_t choice;
        std::cin >> choice;
        
        if (choice == 0 || choice > lights.size()) {
            return;
        }
        
        auto& light = lights[choice - 1];
        auto caps = light.getCapabilities();
        
        bool controlling = true;
        while (controlling) {
            printHeader("Controlling: " + light.getName());
            
            std::cout << "Current State:" << std::endl;
            std::cout << "  Power: " << (light.isOn() ? "ON" : "OFF") << std::endl;
            
            if (caps.brightness) {
                auto brightness = light.getBrightness();
                if (brightness.has_value()) {
                    std::cout << "  Brightness: " << static_cast<int>(brightness.value()) << "%" << std::endl;
                }
            }
            
            if (caps.color) {
                auto color = light.getColor();
                if (color.has_value()) {
                    std::cout << "  Color (XY): x=" << std::fixed << std::setprecision(4) 
                              << color->x << ", y=" << color->y << std::endl;
                }
            }
            
            if (caps.color_temperature) {
                auto temp = light.getColorTemperature();
                if (temp.has_value()) {
                    std::cout << "  Color Temp: " << temp->toKelvin() << "K" << std::endl;
                }
            }
            
            std::cout << std::endl;
            printLine('-');
            std::cout << "Controls:" << std::endl;
            std::cout << "  1. Turn On" << std::endl;
            std::cout << "  2. Turn Off" << std::endl;
            std::cout << "  3. Toggle" << std::endl;
            
            if (caps.brightness) {
                std::cout << "  4. Set Brightness" << std::endl;
            }
            
            if (caps.color) {
                std::cout << "  5. Set Color (RGB)" << std::endl;
            }
            
            if (caps.color_temperature) {
                std::cout << "  6. Set Color Temperature" << std::endl;
            }
            
            std::cout << "  7. Alert (Blink)" << std::endl;
            std::cout << "  8. Refresh State" << std::endl;
            std::cout << "  0. Back to Main Menu" << std::endl;
            printLine('-');
            
            std::cout << std::endl << "Choose: ";
            int control_choice;
            std::cin >> control_choice;
            
            try {
                Result<void> result;
                
                switch (control_choice) {
                case 0:
                    controlling = false;
                    break;
                    
                case 1:
                    result = light.turnOn();
                    if (result.isSuccess()) {
                        std::cout << "✓ Light turned on" << std::endl;
                    } else {
                        std::cout << "✗ Error: " << result.error_message << std::endl;
                    }
                    waitForEnter();
                    break;
                    
                case 2:
                    result = light.turnOff();
                    if (result.isSuccess()) {
                        std::cout << "✓ Light turned off" << std::endl;
                    } else {
                        std::cout << "✗ Error: " << result.error_message << std::endl;
                    }
                    waitForEnter();
                    break;
                    
                case 3:
                    result = light.toggle();
                    if (result.isSuccess()) {
                        std::cout << "✓ Light toggled" << std::endl;
                    } else {
                        std::cout << "✗ Error: " << result.error_message << std::endl;
                    }
                    waitForEnter();
                    break;
                    
                case 4:
                    if (caps.brightness) {
                        std::cout << "Enter brightness (0-100): ";
                        int brightness;
                        std::cin >> brightness;
                        if (brightness >= 0 && brightness <= 100) {
                            result = light.setBrightness(static_cast<uint8_t>(brightness));
                            if (result.isSuccess()) {
                                std::cout << "✓ Brightness set to " << brightness << "%" << std::endl;
                            } else {
                                std::cout << "✗ Error: " << result.error_message << std::endl;
                            }
                        } else {
                            std::cout << "✗ Invalid brightness value" << std::endl;
                        }
                        waitForEnter();
                    }
                    break;
                    
                case 5:
                    if (caps.color) {
                        std::cout << "Enter Red (0-255): ";
                        int r;
                        std::cin >> r;
                        std::cout << "Enter Green (0-255): ";
                        int g;
                        std::cin >> g;
                        std::cout << "Enter Blue (0-255): ";
                        int b;
                        std::cin >> b;
                        
                        result = light.setColor(static_cast<float>(r), 
                                               static_cast<float>(g), 
                                               static_cast<float>(b));
                        if (result.isSuccess()) {
                            std::cout << "✓ Color set to RGB(" << r << "," << g << "," << b << ")" << std::endl;
                        } else {
                            std::cout << "✗ Error: " << result.error_message << std::endl;
                        }
                        waitForEnter();
                    }
                    break;
                    
                case 6:
                    if (caps.color_temperature) {
                        std::cout << "Enter color temperature in Kelvin (2000-6500): ";
                        int kelvin;
                        std::cin >> kelvin;
                        
                        try {
                            auto temp = ColorTemperature::fromKelvin(static_cast<uint16_t>(kelvin));
                            result = light.setColorTemperature(temp);
                            if (result.isSuccess()) {
                                std::cout << "✓ Color temperature set to " << kelvin << "K" << std::endl;
                            } else {
                                std::cout << "✗ Error: " << result.error_message << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cout << "✗ Error: " << e.what() << std::endl;
                        }
                        waitForEnter();
                    }
                    break;
                    
                case 7:
                    result = light.alert();
                    if (result.isSuccess()) {
                        std::cout << "✓ Alert sent (light will blink)" << std::endl;
                    } else {
                        std::cout << "✗ Error: " << result.error_message << std::endl;
                    }
                    waitForEnter();
                    break;
                    
                case 8:
                    result = light.refresh();
                    if (result.isSuccess()) {
                        std::cout << "✓ State refreshed" << std::endl;
                    } else {
                        std::cout << "✗ Error: " << result.error_message << std::endl;
                    }
                    waitForEnter();
                    break;
                    
                default:
                    std::cout << "Invalid option" << std::endl;
                    waitForEnter();
                }
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
                waitForEnter();
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        waitForEnter();
    }
}

// ============================================================================
// Page: List All Sensors
// ============================================================================

void showSensorsPage(AppState& state) {
    printHeader("All Sensors");
    
    if (!state.connected || !state.bridge) {
        std::cout << "Not connected to a bridge." << std::endl;
        waitForEnter();
        return;
    }
    
    try {
        std::cout << "Fetching sensors..." << std::endl;
        auto sensors = state.bridge->getSensors();
        
        if (sensors.empty()) {
            std::cout << std::endl << "No sensors found." << std::endl;
        } else {
            std::cout << std::endl << "Found " << sensors.size() << " sensor(s):" << std::endl;
            std::cout << std::endl;
            
            for (size_t i = 0; i < sensors.size(); i++) {
                const auto& sensor = sensors[i];
                
                std::cout << (i + 1) << ". Sensor" << std::endl;
                std::cout << "   ID: " << sensor->getId() << std::endl;
                std::cout << "   Type: " << sensorTypeToString(sensor->getType()) << std::endl;
                std::cout << "   Enabled: " << (sensor->isEnabled() ? "Yes" : "No") << std::endl;
                std::cout << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    waitForEnter();
}

// ============================================================================
// Page: Sensor Details
// ============================================================================

void showSensorDetailsPage(AppState& state) {
    printHeader("Sensor Details");
    
    if (!state.connected || !state.bridge) {
        std::cout << "Not connected to a bridge." << std::endl;
        waitForEnter();
        return;
    }
    
    try {
        auto sensors = state.bridge->getSensors();
        
        if (sensors.empty()) {
            std::cout << "No sensors found." << std::endl;
            waitForEnter();
            return;
        }
        
        std::cout << "Select a sensor:" << std::endl;
        for (size_t i = 0; i < sensors.size(); i++) {
            std::cout << "  " << (i + 1) << ". " << sensorTypeToString(sensors[i]->getType()) 
                      << " (" << sensors[i]->getId() << ")" << std::endl;
        }
        std::cout << "  0. Back" << std::endl;
        std::cout << std::endl << "Choose: ";
        
        size_t choice;
        std::cin >> choice;
        
        if (choice == 0 || choice > sensors.size()) {
            return;
        }
        
        const auto& sensor = sensors[choice - 1];
        
        printHeader("Sensor Details");
        
        std::cout << "ID: " << sensor->getId() << std::endl;
        std::cout << "Type: " << sensorTypeToString(sensor->getType()) << std::endl;
        std::cout << "Enabled: " << (sensor->isEnabled() ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
        
        // Show type-specific data
        if (const auto* motion_sensor = dynamic_cast<const MotionSensor*>(sensor.get())) {
            auto state = motion_sensor->getMotionState();
            std::cout << "Motion State:" << std::endl;
            std::cout << "  Motion Detected: " << (state.motion ? "YES" : "NO") << std::endl;
            std::cout << "  Data Valid: " << (state.motion_valid ? "Yes" : "No") << std::endl;
        }
        else if (const auto* temp_sensor = dynamic_cast<const TemperatureSensor*>(sensor.get())) {
            auto state = temp_sensor->getTemperatureState();
            std::cout << "Temperature State:" << std::endl;
            std::cout << "  Temperature: " << std::fixed << std::setprecision(2) 
                      << state.temperature << " °C" << std::endl;
            std::cout << "  Data Valid: " << (state.temperature_valid ? "Yes" : "No") << std::endl;
        }
        else if (const auto* light_sensor = dynamic_cast<const LightLevelSensor*>(sensor.get())) {
            auto state = light_sensor->getLightLevelState();
            std::cout << "Light Level State:" << std::endl;
            std::cout << "  Light Level: " << state.light_level << std::endl;
            std::cout << "  Data Valid: " << (state.light_level_valid ? "Yes" : "No") << std::endl;
        }
        else if (const auto* button_sensor = dynamic_cast<const ButtonSensor*>(sensor.get())) {
            auto state = button_sensor->getButtonState();
            std::cout << "Button State:" << std::endl;
            std::cout << "  Button ID: " << state.button_id << std::endl;
            std::cout << "  Event Sequence: " << state.event_sequence << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    waitForEnter();
}

// ============================================================================
// Page: Live Activity Monitor
// ============================================================================

void showLiveActivityPage(AppState& state) {
    printHeader("Live Activity Monitor");
    
    if (!state.connected || !state.bridge) {
        std::cout << "Not connected to a bridge." << std::endl;
        waitForEnter();
        return;
    }
    
    std::cout << "Starting real-time event monitoring..." << std::endl;
    std::cout << "Try controlling lights or triggering sensors via the Hue app." << std::endl;
    std::cout << std::endl;
    std::cout << "Press Enter to stop monitoring..." << std::endl;
    printLine('-');
    std::cout << std::endl;
    
    try {
        auto& state_manager = state.bridge->getStateManager();
        
        // Register callback for events
        auto callback_id = state_manager.registerCallback([](const Event& event) {
            std::cout << "[" << getCurrentTime() << "] " 
                      << eventTypeToString(event.type);
            
            if (!event.resource_id.empty()) {
                std::cout << " - ID: " << event.resource_id;
            }
            
            std::cout << std::endl;
            
            // Show some event data for interesting events
            if (event.type == EventType::LightStateChanged && !event.data.empty()) {
                try {
                    auto json = nlohmann::json::parse(event.data);
                    if (json.contains("on") && json["on"].contains("on")) {
                        std::cout << "    Power: " << (json["on"]["on"].get<bool>() ? "ON" : "OFF") << std::endl;
                    }
                    if (json.contains("dimming") && json["dimming"].contains("brightness")) {
                        std::cout << "    Brightness: " << json["dimming"]["brightness"].get<double>() << "%" << std::endl;
                    }
                } catch (...) {
                    // Ignore parsing errors
                }
            }
        });
        
        state.monitoring = true;
        state.event_callback_id = callback_id;
        
        // Wait for Enter key
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
        
        // Cleanup
        state_manager.unregisterCallback(callback_id);
        state.monitoring = false;
        
        std::cout << std::endl << "Monitoring stopped." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    waitForEnter();
}

// ============================================================================
// Main Application Loop
// ============================================================================

int main() {
    AppState state;
    
    std::cout << "========================================" << std::endl;
    std::cout << "  HUE4CPP Interactive Demo" << std::endl;
    std::cout << "  Version " << Version::STRING << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "This is a comprehensive demo application that showcases" << std::endl;
    std::cout << "all features of the hue4cpp library." << std::endl;
    std::cout << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "  - Bridge discovery and authentication" << std::endl;
    std::cout << "  - Persistent credential storage" << std::endl;
    std::cout << "  - Complete light control" << std::endl;
    std::cout << "  - Sensor monitoring" << std::endl;
    std::cout << "  - Real-time event streaming" << std::endl;
    std::cout << std::endl;
    
    waitForEnter();
    
    bool running = true;
    while (running) {
        int choice = showMainMenu(state);
        
        switch (choice) {
        case 0:
            running = false;
            break;
            
        case 1:
            showConnectPage(state);
            break;
            
        case 2:
            showLightsPage(state);
            break;
            
        case 3:
            showLightControlPage(state);
            break;
            
        case 4:
            showSensorsPage(state);
            break;
            
        case 5:
            showSensorDetailsPage(state);
            break;
            
        case 6:
            showLiveActivityPage(state);
            break;
            
        default:
            std::cout << "Invalid option. Please try again." << std::endl;
            waitForEnter();
        }
    }
    
    // Cleanup
    if (state.monitoring && state.bridge) {
        auto& state_manager = state.bridge->getStateManager();
        state_manager.unregisterCallback(state.event_callback_id);
        state_manager.stop();
    }
    
    printHeader("Goodbye!");
    std::cout << "Thank you for using hue4cpp!" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
