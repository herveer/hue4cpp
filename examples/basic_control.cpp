#include <hue4cpp/hue4cpp.h>
#include <iostream>

/**
 * @brief Basic example showing how to control Hue lights
 * 
 * This example demonstrates:
 * - Discovering bridges
 * - Authenticating with a bridge
 * - Getting lights
 * - Controlling light state, brightness, and color
 */

int main() {
    std::cout << "hue4cpp - Basic Control Example" << std::endl;
    std::cout << "Version: " << hue4cpp::Version::STRING << std::endl;
    std::cout << std::endl;
    
    // Discover bridges on the network
    std::cout << "Discovering bridges..." << std::endl;
    auto bridges = hue4cpp::Bridge::discover();
    
    if (bridges.empty()) {
        std::cerr << "No bridges found!" << std::endl;
        std::cerr << "Make sure your Hue bridge is connected to the same network." << std::endl;
        return 1;
    }
    
    std::cout << "Found " << bridges.size() << " bridge(s)" << std::endl;
    
    // Use the first bridge
    auto& bridge = bridges[0];
    const auto& info = bridge.getInfo();
    std::cout << "Using bridge: " << info.name << " (" << info.ip_address << ")" << std::endl;
    
    // Authenticate (you'll need to press the button on the bridge)
    if (!bridge.isAuthenticated()) {
        std::cout << std::endl;
        std::cout << "Please press the button on your Hue bridge..." << std::endl;
        std::cout << "Press Enter when ready...";
        std::cin.get();
        
        auto auth_result = bridge.authenticate("hue4cpp-example", "computer");
        if (!auth_result.isSuccess()) {
            std::cerr << "Authentication failed: " << auth_result.error_message << std::endl;
            return 1;
        }
        
        std::cout << "Authentication successful!" << std::endl;
        std::cout << "Save this key for future use: " << auth_result.value.value() << std::endl;
    }
    
    // Get all lights
    std::cout << std::endl;
    std::cout << "Getting lights..." << std::endl;
    auto lights = bridge.getLights();
    
    if (lights.empty()) {
        std::cout << "No lights found!" << std::endl;
        return 0;
    }
    
    std::cout << "Found " << lights.size() << " light(s)" << std::endl;
    
    // Control the first light
    auto& light = lights[0];
    std::cout << std::endl;
    std::cout << "Controlling light: " << light.getName() << " (ID: " << light.getId() << ")" << std::endl;
    
    // Turn on the light
    std::cout << "Turning on..." << std::endl;
    auto result = light.turnOn();
    if (!result.isSuccess()) {
        std::cerr << "Failed to turn on: " << result.error_message << std::endl;
    }
    
    // Set brightness to 100%
    if (light.getCapabilities().brightness) {
        std::cout << "Setting brightness to 100%..." << std::endl;
        result = light.setBrightness(100);
        if (!result.isSuccess()) {
            std::cerr << "Failed to set brightness: " << result.error_message << std::endl;
        }
    }
    
    // Set color to blue (if color is supported)
    if (light.getCapabilities().color) {
        std::cout << "Setting color to blue..." << std::endl;
        result = light.setColor(0.0f, 0.0f, 255.0f);
        if (!result.isSuccess()) {
            std::cerr << "Failed to set color: " << result.error_message << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Example completed!" << std::endl;
    
    return 0;
}
