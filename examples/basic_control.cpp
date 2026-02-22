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
#include <conio.h>
using namespace hue4cpp;

// Constants for key codes
constexpr int KEY_UP = 72;
constexpr int KEY_DOWN = 80;
constexpr int KEY_ENTER = 13;
constexpr int KEY_ESC = 27;

/**
 * @brief Interactive light selection with arrow key navigation
 * @param lights Vector of available lights
 * @return Selected light pointer or nullptr if cancelled
 */
hue4cpp::Light* selectLightInteractive(const std::vector<std::unique_ptr<hue4cpp::Light>>& lights) {
    if (lights.empty()) {
        return nullptr;
    }

    if (lights.size() == 1) {
        return lights[0].get();
    }

    int selected = 0;
    bool selecting = true;

    auto printMenu = [&]() {
        system("cls");
        std::cout << "|-----------------------------------------|\n";
        std::cout << "|   Select a Light (up/down to navigate)    |\n";
        std::cout << "|     (Enter to select, Esc to cancel) |\n";
        std::cout << "|-----------------------------------------|\n\n";

        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            if (static_cast<int>(i) == selected) {
                std::cout << " -> ";
            }
            else {
                std::cout << "    ";
            }

            std::cout << "[" << std::setw(2) << (i + 1) << "] "
                << std::left << std::setw(30) << light->Name
                << " (ID: " << light->Id << ")";

            if (static_cast<int>(i) == selected) {
                std::cout << " <-";
            }
            std::cout << "\n";
        }

        std::cout << "\n";
    };

    printMenu();

    while (selecting) {
        int key = _getch();

        if (key == 0xE0 || key == 0) {
            // Extended key sequence
            key = _getch();
            if (key == KEY_UP) {
                selected = (selected > 0) ? selected - 1 : static_cast<int>(lights.size()) - 1;
                printMenu();
            }
            else if (key == KEY_DOWN) {
                selected = (selected < static_cast<int>(lights.size()) - 1) ? selected + 1 : 0;
                printMenu();
            }
        }
        else if (key == KEY_ENTER) {
            selecting = false;
        }
        else if (key == KEY_ESC) {
            return nullptr;
        }
    }

    std::cout << "\nSelected: " << lights[selected]->Name << "\n\n";
    return lights[selected].get();
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

        if (saved_bridge_id == lower_case_bridge_id) {
            return key;
        }

        return "";
    }
    catch (...) {
        return "";
    }
}

/**
 * @brief Basic example showing how to control Hue lights
 * 
 * This example demonstrates:
 * - Discovering bridges
 * - Authenticating with a bridge
 * - Getting lights
 * - Interactive light selection
 * - Controlling light state, brightness, and color
 */

int main() {
    std::cout << "hue4cpp - Basic Control Example" << std::endl;
    std::cout << "Version: " << hue4cpp::Version::STRING << std::endl;
    std::cout << std::endl;
    

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
    
    // Get all lights
    std::cout << std::endl;
    std::cout << "Getting lights..." << std::endl;
    auto lights = bridge.getLights();
    
    if (lights.empty()) {
        std::cout << "No lights found!" << std::endl;
        return 0;
    }
    
    std::cout << "Found " << lights.size() << " light(s)" << std::endl;
    
    // Interactive light selection
    auto light = selectLightInteractive(lights);
    if (!light) {
        std::cout << "Light selection cancelled." << std::endl;
        return 0;
    }
    
    std::cout << "Controlling light: " << light->Name << " (ID: " << light->Id << ")" << std::endl;
    
    // Turn on the light
    std::cout << "Turning on..." << std::endl;
    try {
        light->IsOn = true;
        std::cout << "Light turned on!" << std::endl;
    } catch (const hue4cpp::InvalidParameterException& e) {
        std::cerr << "Failed to turn on: " << e.what() << std::endl;
    } catch (const hue4cpp::HueException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    // Set brightness to 100%
    if (light->getCapabilities().brightness) {
        std::cout << "Setting brightness to 100%..." << std::endl;
        try {
            light->Brightness = 100;
            std::cout << "Brightness set to 100%!" << std::endl;
        } catch (const hue4cpp::InvalidParameterException& e) {
            std::cerr << "Failed to set brightness: " << e.what() << std::endl;
        } catch (const hue4cpp::HueException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    
    // Set color to blue (if color is supported)
    if (light->getCapabilities().color) {
        std::cout << "Setting color to blue..." << std::endl;
        try {
            light->RGBColor_ = hue4cpp::RGBColor(0.0f, 0.0f, 255.0f);
            std::cout << "Color set to blue!" << std::endl;
        } catch (const hue4cpp::InvalidParameterException& e) {
            std::cerr << "Failed to set color: " << e.what() << std::endl;
        } catch (const hue4cpp::HueException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Example completed!" << std::endl;
    
    return 0;
}
