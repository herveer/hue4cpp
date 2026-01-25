/**
 * @file state_monitoring.cpp
 * @brief Example demonstrating real-time state synchronization with SSE
 * 
 * This example shows how to:
 * 1. Connect to a Hue bridge
 * 2. Start real-time state monitoring via Server-Sent Events
 * 3. Register callbacks for light state changes
 * 4. Handle bridge connection/disconnection events
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

void printEventInfo(const Event& event) {
    std::cout << "[" << getCurrentTime() << "] ";
    
    switch (event.type) {
        case EventType::LightStateChanged:
            std::cout << "🔄 Light state changed: " << event.resource_id << "\n";
            if (!event.data.empty()) {
                // Pretty print the light state (simplified)
                std::cout << "   Data: " << event.data.substr(0, 100);
                if (event.data.length() > 100) std::cout << "...";
                std::cout << "\n";
            }
            break;
            
        case EventType::LightAdded:
            std::cout << "➕ Light added: " << event.resource_id << "\n";
            break;
            
        case EventType::LightRemoved:
            std::cout << "➖ Light removed: " << event.resource_id << "\n";
            break;
            
        case EventType::BridgeConnected:
            std::cout << "✅ Bridge connected\n";
            break;
            
        case EventType::BridgeDisconnected:
            std::cout << "❌ Bridge disconnected\n";
            break;
            
        case EventType::Unknown:
        default:
            std::cout << "❓ Unknown event\n";
            break;
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
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "===========================================\n";
    std::cout << "  Hue4cpp - Real-Time State Monitoring\n";
    std::cout << "===========================================\n\n";
    
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

        
        // Step 3: Get available lights
        std::cout << "Fetching lights...\n";
        auto lights = bridge.getLights();
        std::cout << "Found " << lights.size() << " light(s)\n";
        for (const auto& light : lights) {
            std::cout << "   - " << light.getName() << " (" << light.getId() << ")\n";
        }
        std::cout << "\n";
        
        // Step 4: Set up state manager and register callbacks
        std::cout << "Setting up real-time state monitoring...\n";
        auto& state_manager = bridge.getStateManager();
        
        // Register event callback
        auto callback_id = state_manager.registerCallback(printEventInfo);
        
        // Step 5: Start monitoring
        std::cout << "Starting SSE connection...\n";
        auto start_result = state_manager.start();
        if (!start_result) {
            std::cout << "Failed to start state manager: " << start_result.error_message << "\n";
            return 1;
        }
        
        std::cout << "State monitoring active!\n\n";
        std::cout << "===========================================\n";
        std::cout << "  Monitoring light events... (Ctrl+C to stop)\n";
        std::cout << "  Try changing lights via the Hue app!\n";
        std::cout << "===========================================\n\n";
        
        // Step 6: Keep running until interrupted
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Step 7: Clean shutdown
        std::cout << "\nStopping state monitoring...\n";
        state_manager.stop();
        state_manager.unregisterCallback(callback_id);
        
        std::cout << "Shutdown complete.\n";
        
    } catch (const HueException& e) {
        std::cout << "Hue error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
