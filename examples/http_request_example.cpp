/**
 * @file http_request_example.cpp
 * @brief Example demonstrating HTTP requests to a Hue bridge
 * 
 * This example shows how to use the HttpClient to make requests
 * to a Philips Hue bridge. It demonstrates:
 * - Creating an HTTP client
 * - Making GET requests
 * - Parsing JSON responses
 * - Error handling
 */

#include <hue4cpp/http_client.h>
#include <hue4cpp/json_utils.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <bridge_ip> [api_key]" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " 192.168.1.100" << std::endl;
        std::cout << "  " << argv[0] << " 192.168.1.100 your-api-key-here" << std::endl;
        std::cout << std::endl;
        std::cout << "This example demonstrates basic HTTP requests to a Hue bridge." << std::endl;
        std::cout << "Without an API key, it will only access public endpoints." << std::endl;
        return 1;
    }
    
    std::string bridge_ip = argv[1];
    std::string api_key = (argc >= 3) ? argv[2] : "";
    
    std::cout << "Hue4cpp HTTP Request Example" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Bridge IP: " << bridge_ip << std::endl;
    
    // Create HTTP client
    hue4cpp::HttpClient client;
    
    // Configure timeout (10 seconds)
    client.setTimeout(std::chrono::milliseconds(10000));
    
    // For local development, you might want to disable SSL verification
    // In production, always keep SSL verification enabled
    client.setVerifySsl(false);
    
    // Test 1: Get bridge configuration (public endpoint)
    std::cout << std::endl << "Test 1: Getting bridge configuration..." << std::endl;
    std::string config_url = "https://" + bridge_ip + "/api/0/config";
    
    auto response = client.get(config_url);
    
    if (response.isSuccess()) {
        std::cout << "Success! Status code: " << response.status_code << std::endl;
        
        try {
            auto json = hue4cpp::json_utils::parse(response.body);
            std::cout << "Response (formatted):" << std::endl;
            std::cout << hue4cpp::json_utils::toString(json, 2) << std::endl;
            
            // Try to extract some information
            auto name = hue4cpp::json_utils::getValue<std::string>(json, "name");
            if (name) {
                std::cout << "Bridge name: " << name.value() << std::endl;
            }
            
            auto sw_version = hue4cpp::json_utils::getValue<std::string>(json, "swversion");
            if (sw_version) {
                std::cout << "Software version: " << sw_version.value() << std::endl;
            }
        } catch (const hue4cpp::JsonParseException& e) {
            std::cout << "Failed to parse JSON response: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Failed! Status code: " << response.status_code << std::endl;
        if (!response.error_message.empty()) {
            std::cout << "Error message: " << response.error_message << std::endl;
        }
        std::cout << "Response body: " << response.body << std::endl;
    }
    
    // Test 2: If we have an API key, try to get lights
    if (!api_key.empty()) {
        std::cout << std::endl << "Test 2: Getting lights (requires authentication)..." << std::endl;
        std::string lights_url = "https://" + bridge_ip + "/api/" + api_key + "/lights";
        
        auto lights_response = client.get(lights_url);
        
        if (lights_response.isSuccess()) {
            std::cout << "Success! Status code: " << lights_response.status_code << std::endl;
            
            try {
                auto json = hue4cpp::json_utils::parse(lights_response.body);
                std::cout << "Response (formatted):" << std::endl;
                std::cout << hue4cpp::json_utils::toString(json, 2) << std::endl;
            } catch (const hue4cpp::JsonParseException& e) {
                std::cout << "Failed to parse JSON response: " << e.what() << std::endl;
            }
        } else {
            std::cout << "Failed! Status code: " << lights_response.status_code << std::endl;
            if (!lights_response.error_message.empty()) {
                std::cout << "Error message: " << lights_response.error_message << std::endl;
            }
            std::cout << "Response body: " << lights_response.body << std::endl;
        }
    } else {
        std::cout << std::endl << "Skipping authenticated requests (no API key provided)" << std::endl;
    }
    
    std::cout << std::endl << "Example completed!" << std::endl;
    return 0;
}
