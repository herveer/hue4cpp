#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>

/**
 * @brief Example demonstrating color control features
 *
 * This example shows:
 * - Using preset colors from the color palette
 * - RGB to XY color conversion
 * - Color temperature control
 * - HSV color space manipulation
 * - Smooth color transitions
 */

#ifdef _WIN32
#include <conio.h>

enum Key { KEY_NONE, KEY_UP, KEY_DOWN, KEY_ENTER };

Key readKey() {
	int ch = _getch();
	if (ch == 224) { // arrow prefix
		ch = _getch();
		if (ch == 72) return KEY_UP;
		if (ch == 80) return KEY_DOWN;
	}
	if (ch == 13) return KEY_ENTER;
	return KEY_NONE;
}
#else
#include <termios.h>
#include <unistd.h>

enum Key { KEY_NONE, KEY_UP, KEY_DOWN, KEY_ENTER };

Key readKey() {
	char c;
	read(STDIN_FILENO, &c, 1);

	if (c == '\n') return KEY_ENTER;

	if (c == 27) { // escape sequence
		char seq[2];
		read(STDIN_FILENO, &seq[0], 1);
		read(STDIN_FILENO, &seq[1], 1);
		if (seq[1] == 'A') return KEY_UP;
		if (seq[1] == 'B') return KEY_DOWN;
	}
	return KEY_NONE;
}

struct TermGuard {
	termios oldt;
	TermGuard() {
		tcgetattr(STDIN_FILENO, &oldt);
		termios newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	}
	~TermGuard() {
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	}
};
#endif


using namespace hue4cpp;
using namespace std::chrono_literals;

void printColorNames() {
	std::cout << "\nAvailable preset colors:" << std::endl;
	auto names = colors::getColorNames();
	for (size_t i = 0; i < names.size(); ++i) {
		std::cout << "  " << names[i];
		if ((i + 1) % 5 == 0) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}

void demonstrateBrightnessControl(Light& light) {
	std::cout << "\n=== Demonstrating Brightness Control ===" << std::endl;
	try {
		// Turn on the light first
		light.IsOn = true;
		std::this_thread::sleep_for(500ms);
		// Gradually increase brightness from 0% to 100%
		std::cout << "Increasing brightness from 0% to 100%..." << std::endl;
		for (uint8_t b = 0; b <= 100; b += 10) {
			light.TransitionTime_ = std::chrono::milliseconds(500);
			light.Brightness = b;
			std::this_thread::sleep_for(1s);
		}
		// Gradually decrease brightness from 100% to 0%
		std::cout << "Decreasing brightness from 100% to 0%..." << std::endl;
		for (int b = 100; b >= 0; b -= 10) {
			light.Brightness = static_cast<uint8_t>(b);
			std::this_thread::sleep_for(1s);
		}
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during brightness control: " << e.what() << std::endl;
	}
}

void demonstrateAlertEffect(Light& light) {
	std::cout << "\n=== Demonstrating Alert Effect ===" << std::endl;
	try {
		// Turn on the light first
		light.IsOn = true;
		std::this_thread::sleep_for(500ms);
		// Note: alert() would need to be called directly or through a separate method
		// For now, we'll just demonstrate on/off toggling
		std::cout << "Demonstrating on/off toggle (alert would require dedicated method)..." << std::endl;
		light.IsOn = false;
		std::this_thread::sleep_for(1s);
		light.IsOn = true;
		std::this_thread::sleep_for(1s);
		light.IsOn = false;
		std::this_thread::sleep_for(500ms);
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during alert effect: " << e.what() << std::endl;
	}
}

void demonstratePresetColors(Light& light) {
	std::cout << "\n=== Demonstrating Preset Colors ===" << std::endl;

	try {
		// Turn on the light first
		light.IsOn = true;
		std::this_thread::sleep_for(500ms);

		// Try different preset colors
		std::vector<std::string> demo_colors = { "Red", "Green", "Blue", "Orange", "Purple" };

		for (const auto& color_name : demo_colors) {
			auto color = colors::getColorByName(color_name);
			if (color.has_value()) {
				std::cout << "Setting color to " << color_name << "..." << std::endl;
				light.TransitionTime_ = std::chrono::milliseconds(1000);
				light.RGBColor_ = color.value();
				std::this_thread::sleep_for(2s);
			}
		}
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during color demonstration: " << e.what() << std::endl;
	}
}

void demonstrateColorTemperature(Light& light) {
	std::cout << "\n=== Demonstrating Color Temperature ===" << std::endl;

	if (!light.getCapabilities().color_temperature) {
		std::cout << "This light does not support color temperature control." << std::endl;
		return;
	}

	try {
		// Warm white (2700K)
		std::cout << "Setting warm white (2700K)..." << std::endl;
		auto warm = ColorTemperature::fromKelvin(2700);
		light.TransitionTime_ = std::chrono::milliseconds(1000);
		light.ColorTemperature_ = warm;
		std::this_thread::sleep_for(3s);

		// Neutral white (4000K)
		std::cout << "Setting neutral white (4000K)..." << std::endl;
		auto neutral = ColorTemperature::fromKelvin(4000);
		light.ColorTemperature_ = neutral;
		std::this_thread::sleep_for(3s);

		// Cool white (6500K)
		std::cout << "Setting cool white (6500K)..." << std::endl;
		auto cool = ColorTemperature::fromKelvin(6500);
		light.ColorTemperature_ = cool;
		std::this_thread::sleep_for(3s);
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during color temperature demonstration: " << e.what() << std::endl;
	}
}

void demonstrateHSVColors(Light& light) {
	std::cout << "\n=== Demonstrating HSV Color Space ===" << std::endl;

	if (!light.getCapabilities().color) {
		std::cout << "This light does not support color control." << std::endl;
		return;
	}

	try {
		// Create a rainbow effect by varying hue
		std::cout << "Creating rainbow effect..." << std::endl;
		light.TransitionTime_ = std::chrono::milliseconds(500);
		for (int hue = 0; hue < 360; hue += 30) {
			RGBColor rgb = color_utils::hsvToRgb(static_cast<float>(hue), 100.0f, 100.0f);
			light.RGBColor_ = rgb;
			std::this_thread::sleep_for(1s);
		}
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during HSV color demonstration: " << e.what() << std::endl;
	}
}

void demonstrateCustomColors(Light& light) {
	std::cout << "\n=== Demonstrating Custom RGB Colors ===" << std::endl;

	if (!light.getCapabilities().color) {
		std::cout << "This light does not support color control." << std::endl;
		return;
	}

	try {
		// Define some custom colors
		struct NamedColor {
			std::string name;
			RGBColor color;
		};

		std::vector<NamedColor> custom_colors = {
			{"Coral", RGBColor(255, 127, 80)},
			{"Turquoise", RGBColor(64, 224, 208)},
			{"Gold", RGBColor(255, 215, 0)},
			{"Lavender", RGBColor(230, 230, 250)},
			{"Salmon", RGBColor(250, 128, 114)}
		};

		light.TransitionTime_ = std::chrono::milliseconds(1000);
		for (const auto& nc : custom_colors) {
			std::cout << "Setting color to " << nc.name << " (RGB: "
				<< nc.color.r << ", " << nc.color.g << ", " << nc.color.b << ")..."
				<< std::endl;
			light.RGBColor_ = nc.color;
			std::this_thread::sleep_for(2s);
		}
	} catch (const hue4cpp::HueException& e) {
		std::cerr << "Error during custom color demonstration: " << e.what() << std::endl;
	}
}


// Simple file-based key storage for demonstration
// In production, use OS keychain integration for better security
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
		if (saved_bridge_id == bridge_id) {
			return key;
		}

		return "";
	}
	catch (...) {
		return "";
	}
}

int main() {
	std::cout << "hue4cpp - Color Control Example" << std::endl;
	std::cout << "Version: " << Version::STRING << std::endl;
	std::cout << std::endl;

	// Show available preset colors
	printColorNames();

	// Discover bridges
	std::cout << "\nDiscovering bridges..." << std::endl;
	auto bridges = Bridge::discover();

	if (bridges.empty()) {
		std::cerr << "No bridges found!" << std::endl;
		return 1;
	}

	std::cout << "Found " << bridges.size() << " bridge(s)" << std::endl;

	// Use the first bridge
	auto& bridge = bridges[0];
	const auto& info = bridge.getInfo();
	std::cout << "Using bridge: " << info.name << " (" << info.ip_address << ")" << std::endl;

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

	// Get lights
	std::cout << "\nGetting lights..." << std::endl;
	auto lights = bridge.getLights();

	if (lights.empty()) {
		std::cout << "No lights found!" << std::endl;
		return 0;
	}

	std::cout << "Found " << lights.size() << " light(s)" << std::endl;

	// Use the first light that supports color
	for (auto& light : lights) {
		std::cout << "  - " << light->getName() << " (ID: " << light->getId() << ")" << std::endl;
		auto caps = light->getCapabilities();
		std::cout << "    Capabilities: ";
		if (caps.brightness) std::cout << "brightness ";
		if (caps.color) std::cout << "color ";
		if (caps.color_temperature) std::cout << "color_temp ";
		std::cout << std::endl;
	}

	// Press enter to select a color-capable light
	std::cout << "\nPress Enter to select a color-capable light to test";
	std::cin.get();

	std::vector<Light*> colorLights;

	for (auto& light : lights) {
		auto caps = light->getCapabilities();
		if (caps.color) {
			colorLights.push_back(light.get());
		}
	}

	if (colorLights.empty()) {
		std::cout << "No color-capable lights found!\n";
		return 0;
	}

	int selected = 0;

#ifndef _WIN32
	TermGuard guard; // enable raw mode
#endif

	while (true) {
#ifdef _WIN32
		system("cls"); // Windows: system("cls")
#else
		system("clear"); // UNIX: system("clear")
#endif

		std::cout << "Select a color-capable light:\n\n";

		for (size_t i = 0; i < colorLights.size(); ++i) {
			if ((int)i == selected)
				std::cout << " > ";
			else
				std::cout << "   ";

			std::cout << colorLights[i]->getName()
				<< " (ID: " << colorLights[i]->getId() << ")\n";
		}

		Key key = readKey();

		if (key == KEY_UP && selected > 0)
			selected--;
		else if (key == KEY_DOWN && selected < (int)colorLights.size() - 1)
			selected++;
		else if (key == KEY_ENTER)
			break;
	}

	Light* color_light = colorLights[selected];
	std::cout << "\nUsing light: " << color_light->getName() << std::endl;

	// Run demonstrations
	try {
		demonstrateAlertEffect(*color_light);
		demonstrateBrightnessControl(*color_light);
		demonstratePresetColors(*color_light);
		demonstrateColorTemperature(*color_light);
		demonstrateHSVColors(*color_light);
		demonstrateCustomColors(*color_light);

		// Return to neutral white
		std::cout << "\nReturning to neutral white..." << std::endl;
		color_light->ColorTemperature_ = hue4cpp::ColorTemperature::fromKelvin(4000);
		color_light->Brightness = 100;

	}
	catch (const std::exception& e) {
		std::cerr << "Error during demonstration: " << e.what() << std::endl;
		return 1;
	}

	std::cout << "\nExample completed!" << std::endl;
	return 0;
}
