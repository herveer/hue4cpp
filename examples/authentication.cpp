#include <hue4cpp/hue4cpp.h>
#include <iostream>
#include <fstream>
#include <string>

/**
 * @brief Authentication example showing how to authenticate with a Hue bridge
 * 
 * This example demonstrates:
 * - Discovering a bridge
 * - Interactive authentication flow (link button press)
 * - Saving authentication key to a file
 * - Loading and reusing a saved authentication key
 * - Validating an existing authentication key
 */

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
    } catch (...) {
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
    } catch (...) {
        return "";
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  hue4cpp - Authentication Example" << std::endl;
    std::cout << "  Version: " << hue4cpp::Version::STRING << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Step 1: Discover bridges
    std::cout << "Step 1: Discovering Hue bridges..." << std::endl;
    auto bridges = hue4cpp::Bridge::discover();
    
    if (bridges.empty()) {
        std::cerr << "Error: No bridges found!" << std::endl;
        std::cerr << "Please ensure your Hue bridge is:" << std::endl;
        std::cerr << "  - Powered on" << std::endl;
        std::cerr << "  - Connected to the same network as this computer" << std::endl;
        return 1;
    }
    
    std::cout << "Found " << bridges.size() << " bridge(s)" << std::endl;
    std::cout << std::endl;
    
    // Step 2: Select a bridge (use the first one for simplicity)
    auto& bridge = bridges[0];
    const auto& info = bridge.getInfo();
    
    std::cout << "Step 2: Selected bridge" << std::endl;
    std::cout << "  Name:       " << info.name << std::endl;
    std::cout << "  IP Address: " << info.ip_address << std::endl;
    std::cout << "  Bridge ID:  " << info.id << std::endl;
    std::cout << "  Model:      " << info.model_id << std::endl;
    std::cout << std::endl;
    
    // Step 3: Try to load saved authentication key
    std::cout << "Step 3: Checking for saved authentication key..." << std::endl;
    std::string saved_key = loadAuthKey(info.id);
    
    if (!saved_key.empty()) {
        std::cout << "Found saved authentication key!" << std::endl;
        bridge.setAuthenticationKey(saved_key);
        
        std::cout << "Validating key with bridge..." << std::endl;
        auto validation_result = bridge.validateAuthentication();
        
        if (validation_result.isSuccess()) {
            std::cout << "✓ Authentication key is valid!" << std::endl;
            std::cout << std::endl;
            std::cout << "You are now authenticated with the bridge." << std::endl;
            std::cout << "You can use this bridge instance to control lights." << std::endl;
            return 0;
        } else {
            std::cout << "✗ Saved key is no longer valid." << std::endl;
            std::cout << "  Error: " << validation_result.error_message << std::endl;
            std::cout << "  Need to re-authenticate..." << std::endl;
            std::cout << std::endl;
        }
    } else {
        std::cout << "No saved key found for this bridge." << std::endl;
        std::cout << std::endl;
    }
    
    // Step 4: Perform authentication
    std::cout << "Step 4: Authenticating with bridge" << std::endl;
    std::cout << std::endl;
    std::cout << "******************************************" << std::endl;
    std::cout << "  ACTION REQUIRED:" << std::endl;
    std::cout << "  Please press the LINK BUTTON on your" << std::endl;
    std::cout << "  Hue bridge within the next 30 seconds" << std::endl;
    std::cout << "******************************************" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Enter after you've pressed the link button...";
    std::cin.get();
    
    std::cout << std::endl;
    std::cout << "Attempting to authenticate..." << std::endl;
    std::cout << "(This may take up to 30 seconds if the button wasn't pressed yet)" << std::endl;
    
    auto auth_result = bridge.authenticate("hue4cpp-auth-example", "my-computer");
    
    if (!auth_result.isSuccess()) {
        std::cerr << std::endl;
        std::cerr << "✗ Authentication failed!" << std::endl;
        std::cerr << "  Error: " << auth_result.error_message << std::endl;
        std::cerr << std::endl;
        std::cerr << "Common causes:" << std::endl;
        std::cerr << "  - Link button was not pressed" << std::endl;
        std::cerr << "  - Too much time elapsed after pressing the button" << std::endl;
        std::cerr << "  - Bridge is not reachable" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Please try again." << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "✓ Authentication successful!" << std::endl;
    std::cout << std::endl;
    
    // Step 5: Save the authentication key
    std::string auth_key = auth_result.value.value();
    std::cout << "Step 5: Saving authentication key" << std::endl;
    std::cout << "  Key: " << auth_key << std::endl;
    
    if (saveAuthKey(info.id, auth_key)) {
        std::cout << "✓ Key saved to: " << KEY_FILE << std::endl;
        std::cout << std::endl;
        std::cout << "IMPORTANT SECURITY NOTE:" << std::endl;
        std::cout << "  This example saves the key to a plain text file for" << std::endl;
        std::cout << "  demonstration purposes only. In production applications," << std::endl;
        std::cout << "  you should use OS keychain integration for secure storage:" << std::endl;
        std::cout << "    - Windows: Credential Manager" << std::endl;
        std::cout << "    - macOS: Keychain" << std::endl;
        std::cout << "    - Linux: Secret Service API / libsecret" << std::endl;
    } else {
        std::cout << "✗ Failed to save key to file" << std::endl;
        std::cout << "  Please save this key manually: " << auth_key << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Authentication Complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "  1. Run this example again to verify saved key works" << std::endl;
    std::cout << "  2. Use the 'basic_control' example to control lights" << std::endl;
    std::cout << "  3. Integrate hue4cpp into your own application" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
