#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <thread>
#include <chrono>

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

void demonstratePresetColors(Light& light) {
    std::cout << "\n=== Demonstrating Preset Colors ===" << std::endl;
    
    // Turn on the light first
    light.turnOn();
    std::this_thread::sleep_for(500ms);
    
    // Try different preset colors
    std::vector<std::string> demo_colors = {"Red", "Green", "Blue", "Orange", "Purple"};
    
    for (const auto& color_name : demo_colors) {
        auto color = colors::getColorByName(color_name);
        if (color.has_value()) {
            std::cout << "Setting color to " << color_name << "..." << std::endl;
            auto result = light.setColor(color.value(), 1000ms);
            if (!result.isSuccess()) {
                std::cerr << "Failed to set color: " << result.error_message << std::endl;
            }
            std::this_thread::sleep_for(2s);
        }
    }
}

void demonstrateColorTemperature(Light& light) {
    std::cout << "\n=== Demonstrating Color Temperature ===" << std::endl;
    
    if (!light.getCapabilities().color_temperature) {
        std::cout << "This light does not support color temperature control." << std::endl;
        return;
    }
    
    // Warm white (2700K)
    std::cout << "Setting warm white (2700K)..." << std::endl;
    auto warm = ColorTemperature::fromKelvin(2700);
    light.setColorTemperature(warm, 1000ms);
    std::this_thread::sleep_for(3s);
    
    // Neutral white (4000K)
    std::cout << "Setting neutral white (4000K)..." << std::endl;
    auto neutral = ColorTemperature::fromKelvin(4000);
    light.setColorTemperature(neutral, 1000ms);
    std::this_thread::sleep_for(3s);
    
    // Cool white (6500K)
    std::cout << "Setting cool white (6500K)..." << std::endl;
    auto cool = ColorTemperature::fromKelvin(6500);
    light.setColorTemperature(cool, 1000ms);
    std::this_thread::sleep_for(3s);
}

void demonstrateHSVColors(Light& light) {
    std::cout << "\n=== Demonstrating HSV Color Space ===" << std::endl;
    
    if (!light.getCapabilities().color) {
        std::cout << "This light does not support color control." << std::endl;
        return;
    }
    
    // Create a rainbow effect by varying hue
    std::cout << "Creating rainbow effect..." << std::endl;
    for (int hue = 0; hue < 360; hue += 30) {
        RGBColor rgb = color_utils::hsvToRgb(static_cast<float>(hue), 100.0f, 100.0f);
        light.setColor(rgb, 500ms);
        std::this_thread::sleep_for(1s);
    }
}

void demonstrateCustomColors(Light& light) {
    std::cout << "\n=== Demonstrating Custom RGB Colors ===" << std::endl;
    
    if (!light.getCapabilities().color) {
        std::cout << "This light does not support color control." << std::endl;
        return;
    }
    
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
    
    for (const auto& nc : custom_colors) {
        std::cout << "Setting color to " << nc.name << " (RGB: " 
                  << nc.color.r << ", " << nc.color.g << ", " << nc.color.b << ")..." 
                  << std::endl;
        auto result = light.setColor(nc.color, 1000ms);
        if (!result.isSuccess()) {
            std::cerr << "Failed to set color: " << result.error_message << std::endl;
        }
        std::this_thread::sleep_for(2s);
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
        std::cout << "\nPlease press the button on your Hue bridge..." << std::endl;
        std::cout << "Press Enter when ready...";
        std::cin.get();
        
        auto auth_result = bridge.authenticate("hue4cpp-color-example", "computer");
        if (!auth_result.isSuccess()) {
            std::cerr << "Authentication failed: " << auth_result.error_message << std::endl;
            return 1;
        }
        
        std::cout << "Authentication successful!" << std::endl;
        std::cout << "Save this key for future use: " << auth_result.value.value() << std::endl;
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
    Light* color_light = nullptr;
    for (auto& light : lights) {
        std::cout << "  - " << light.getName() << " (ID: " << light.getId() << ")" << std::endl;
        auto caps = light.getCapabilities();
        std::cout << "    Capabilities: ";
        if (caps.brightness) std::cout << "brightness ";
        if (caps.color) std::cout << "color ";
        if (caps.color_temperature) std::cout << "color_temp ";
        std::cout << std::endl;
        
        if (caps.color && !color_light) {
            color_light = &light;
        }
    }
    
    if (!color_light) {
        std::cout << "\nNo color-capable lights found!" << std::endl;
        // Use first light anyway for basic demonstrations
        color_light = &lights[0];
    }
    
    std::cout << "\nUsing light: " << color_light->getName() << std::endl;
    
    // Run demonstrations
    try {
        demonstratePresetColors(*color_light);
        demonstrateColorTemperature(*color_light);
        demonstrateHSVColors(*color_light);
        demonstrateCustomColors(*color_light);
        
        // Return to neutral white
        std::cout << "\nReturning to neutral white..." << std::endl;
        color_light->setColor(colors::White, 1000ms);
        color_light->setBrightness(100, 1000ms);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nExample completed!" << std::endl;
    return 0;
}
