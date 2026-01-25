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

int main() {
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "===========================================\n";
    std::cout << "  Hue4cpp - Real-Time State Monitoring\n";
    std::cout << "===========================================\n\n";
    
    try {
        // Step 1: Discover bridges
        std::cout << "🔍 Discovering bridges...\n";
        auto bridges = Bridge::discover();
        
        if (bridges.empty()) {
            std::cout << "❌ No bridges found. Please ensure your bridge is on the network.\n";
            return 1;
        }
        
        std::cout << "✅ Found " << bridges.size() << " bridge(s)\n";
        auto& bridge = bridges[0];
        const auto& info = bridge.getInfo();
        std::cout << "   Bridge: " << info.name << " (" << info.ip_address << ")\n\n";
        
        // Step 2: Check authentication
        std::cout << "🔑 Checking authentication...\n";
        if (!bridge.isAuthenticated()) {
            std::cout << "   Not authenticated. Starting authentication flow...\n";
            std::cout << "   Please press the link button on your bridge now...\n";
            
            auto auth_result = bridge.authenticate("hue4cpp-state-monitor", "example-device");
            if (!auth_result) {
                std::cout << "❌ Authentication failed: " << auth_result.error_message << "\n";
                return 1;
            }
            
            std::cout << "✅ Authentication successful!\n";
            std::cout << "   Key: " << auth_result.value.value() << "\n\n";
        } else {
            std::cout << "✅ Already authenticated\n\n";
        }
        
        // Step 3: Get available lights
        std::cout << "💡 Fetching lights...\n";
        auto lights = bridge.getLights();
        std::cout << "✅ Found " << lights.size() << " light(s)\n";
        for (const auto& light : lights) {
            std::cout << "   - " << light.getName() << " (" << light.getId() << ")\n";
        }
        std::cout << "\n";
        
        // Step 4: Set up state manager and register callbacks
        std::cout << "📡 Setting up real-time state monitoring...\n";
        auto& state_manager = bridge.getStateManager();
        
        // Register event callback
        auto callback_id = state_manager.registerCallback(printEventInfo);
        
        // Step 5: Start monitoring
        std::cout << "▶️  Starting SSE connection...\n";
        auto start_result = state_manager.start();
        if (!start_result) {
            std::cout << "❌ Failed to start state manager: " << start_result.error_message << "\n";
            return 1;
        }
        
        std::cout << "✅ State monitoring active!\n\n";
        std::cout << "===========================================\n";
        std::cout << "  Monitoring light events... (Ctrl+C to stop)\n";
        std::cout << "  Try changing lights via the Hue app!\n";
        std::cout << "===========================================\n\n";
        
        // Step 6: Keep running until interrupted
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Step 7: Clean shutdown
        std::cout << "\n🛑 Stopping state monitoring...\n";
        state_manager.stop();
        state_manager.unregisterCallback(callback_id);
        
        std::cout << "✅ Shutdown complete.\n";
        
    } catch (const HueException& e) {
        std::cout << "❌ Hue error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cout << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
