/**
 * @file rename_demo.cpp
 * @brief Interactive demo for renaming Hue lights and sensors
 *
 * Demonstrates:
 * - Discovering and authenticating with a bridge
 * - Listing all lights and sensors with their current names
 * - Selecting a resource interactively with arrow keys
 * - Renaming it via the writable Name property
 * - Observing the PropertyChanged notification fired after the bridge
 *   acknowledges the rename via SSE
 */

#include <hue4cpp/hue4cpp.h>
#include <hue4cpp/sensors.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <conio.h>

using namespace hue4cpp;

// ── key codes ────────────────────────────────────────────────────────────────
constexpr int KEY_UP    = 72;
constexpr int KEY_DOWN  = 80;
constexpr int KEY_ENTER = 13;
constexpr int KEY_ESC   = 27;

// ── auth helpers (shared pattern with other examples) ────────────────────────
static const std::string KEY_FILE = "hue_auth_key.txt";

static bool saveAuthKey(const std::string& bridge_id, const std::string& key) {
    std::ofstream f(KEY_FILE);
    if (!f.is_open()) return false;
    f << bridge_id << "\n" << key << "\n";
    return true;
}

static std::string loadAuthKey(const std::string& bridge_id) {
    std::ifstream f(KEY_FILE);
    if (!f.is_open()) return "";
    std::string saved_id, key;
    std::getline(f, saved_id);
    std::getline(f, key);
    std::string lower = bridge_id;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (saved_id == lower) ? key : "";
}

static bool authenticate(Bridge& bridge, const BridgeInfo& info) {
    std::string saved = loadAuthKey(info.id);
    if (!saved.empty()) {
        bridge.setAuthenticationKey(saved);
        if (bridge.validateAuthentication().isSuccess()) {
            std::cout << "  Saved key valid.\n";
            return true;
        }
    }
    std::cout << "  Press the button on your bridge, then press Enter...";
    std::cin.get();
    auto result = bridge.authenticate("hue4cpp-rename-demo", "computer");
    if (!result.isSuccess()) {
        std::cerr << "  Authentication failed: " << result.error_message << "\n";
        return false;
    }
    if (result.hasValue())
        saveAuthKey(info.id, result.value.value());
    return true;
}

// ── generic arrow-key menu ────────────────────────────────────────────────────

struct MenuItem {
    std::string label;   // displayed name
    std::string id;      // resource id
    std::string kind;    // "light" or sensor type string
};

/**
 * @brief Shows an arrow-key menu and returns the index chosen, or -1 on Esc.
 */
static int arrowMenu(const std::string& title, const std::vector<MenuItem>& items) {
    if (items.empty()) return -1;

    int selected = 0;

    auto render = [&]() {
        system("cls");
        std::cout << "+--------------------------------------------------+\n";
        std::cout << "|  " << std::left << std::setw(48) << title << "|\n";
        std::cout << "|  (Up/Down navigate  -  Enter select  -  Esc back)|\n";
        std::cout << "+--------------------------------------------------+\n\n";

        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            bool cur = (i == selected);
            std::cout << (cur ? " -> " : "    ")
                      << "[" << std::setw(2) << (i + 1) << "] "
                      << std::left << std::setw(32) << items[i].label
                      << "  (" << items[i].kind << ")"
                      << (cur ? " <-" : "") << "\n";
        }
        std::cout << "\n";
    };

    render();

    while (true) {
        int k = _getch();
        if (k == 0xE0 || k == 0) {
            k = _getch();
            if (k == KEY_UP) {
                selected = (selected > 0)
                    ? selected - 1
                    : static_cast<int>(items.size()) - 1;
                render();
            } else if (k == KEY_DOWN) {
                selected = (selected < static_cast<int>(items.size()) - 1)
                    ? selected + 1
                    : 0;
                render();
            }
        } else if (k == KEY_ENTER) {
            return selected;
        } else if (k == KEY_ESC) {
            return -1;
        }
    }
}

// ── rename helpers ────────────────────────────────────────────────────────────

static void renameLight(Bridge& bridge,
                        const std::vector<std::unique_ptr<Light>>& lights,
                        int idx) {
    Light& light = *lights[idx];
    
    // Subscribe to PropertyChanged before the rename so we can confirm
    // the bridge acknowledged it via the SSE round-trip.
    bool sse_confirmed = false;
    auto sub = light.PropertyChanged.SubscribeScoped(
        [&sse_confirmed](ReactiveLitepp::ObservableObject&,
                         ReactiveLitepp::PropertyChangedArgs args) {
            if (args.PropertyName() == "Name")
                sse_confirmed = true;
        });

    system("cls");
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "|  Rename Light                                    |\n";
    std::cout << "+--------------------------------------------------+\n\n";
    std::cout << "  Current name : " << (std::string)light.Name << "\n";
    std::cout << "  ID           : " << (std::string)light.Id   << "\n\n";
    std::cout << "  New name (Enter to confirm, empty to cancel): ";

    std::string new_name;
    std::getline(std::cin, new_name);

    if (new_name.empty()) {
        std::cout << "\n  Cancelled.\n";
        return;
    }

    try {
        light.Name = new_name;
        sse_confirmed = false;
        std::cout << "\n  PUT sent. New name applied: \"" << (std::string)light.Name << "\"\n";
        if (sse_confirmed)
            std::cout << "  Bridge confirmed rename via SSE.\n";
        else
            std::cout << "  (SSE confirmation pending - bridge will echo shortly)\n";
    } catch (const InvalidParameterException& e) {
        std::cerr << "\n  Invalid: " << e.what() << "\n";
    } catch (const HueException& e) {
        std::cerr << "\n  Error: " << e.what() << "\n";
    }

    std::cout << "\n  Press any key to continue...";
    _getch();
}

static void renameSensor(Bridge& bridge,
                         const std::vector<std::unique_ptr<Sensor>>& sensors,
                         int idx) {
    Sensor& sensor = *sensors[idx];

    bool sse_confirmed = false;
    auto sub = sensor.PropertyChanged.SubscribeScoped(
        [&sse_confirmed](ReactiveLitepp::ObservableObject&,
                         ReactiveLitepp::PropertyChangedArgs args) {
            if (args.PropertyName() == "Name")
                sse_confirmed = true;
        });

    system("cls");
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "|  Rename Sensor                                   |\n";
    std::cout << "+--------------------------------------------------+\n\n";
    std::cout << "  Current name : " << (std::string)sensor.Name << "\n";
    std::cout << "  ID           : " << sensor.getId()           << "\n\n";
    std::cout << "  New name (Enter to confirm, empty to cancel): ";

    std::string new_name;
    std::getline(std::cin, new_name);

    if (new_name.empty()) {
        std::cout << "\n  Cancelled.\n";
        return;
    }

    try {
        sensor.Name = new_name;
        std::cout << "\n  PUT sent. New name applied: \"" << (std::string)sensor.Name << "\"\n";
        if (sse_confirmed)
            std::cout << "  Bridge confirmed rename via SSE.\n";
        else
            std::cout << "  (SSE confirmation pending - bridge will echo shortly)\n";
    } catch (const InvalidParameterException& e) {
        std::cerr << "\n  Invalid: " << e.what() << "\n";
    } catch (const HueException& e) {
        std::cerr << "\n  Error: " << e.what() << "\n";
    }

    std::cout << "\n  Press any key to continue...";
    _getch();
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    system("cls");
    std::cout << "hue4cpp - Rename Demo\n";
    std::cout << "Version: " << hue4cpp::Version::STRING << "\n\n";

    // ── 1. Discover bridge ────────────────────────────────────────────────────
    std::cout << "Discovering bridges...\n";
    auto bridges = Bridge::discover();
    if (bridges.empty()) {
        std::cerr << "No bridges found.\n";
        return 1;
    }

    auto& bridge = bridges[0];
    const auto& info = bridge.getInfo();
    std::cout << "Found: " << info.name << " (" << info.ip_address << ")\n\n";

    // ── 2. Authenticate ───────────────────────────────────────────────────────
    if (!bridge.isAuthenticated()) {
        std::cout << "Authenticating...\n";
        if (!authenticate(bridge, info))
            return 1;
    }

    // ── 3. Start SSE so the bridge can push the confirmed rename back ─────────
    auto& state_manager = bridge.getStateManager();
    state_manager.start();

    // ── 4. Fetch all lights and sensors ──────────────────────────────────────
    std::cout << "\nFetching resources...\n";
    auto lights  = bridge.getLights();
    auto sensors = bridge.getSensors();

    std::cout << "  Lights : " << lights.size()  << "\n";
    std::cout << "  Sensors: " << sensors.size() << "\n\n";

    // ── 5. Main loop ──────────────────────────────────────────────────────────
    while (true) {
        // Build the top-level resource menu
        std::vector<MenuItem> top_items;
        for (const auto& l : lights)
            top_items.push_back({ (std::string)l->Name, (std::string)l->Id, "light" });
        for (const auto& s : sensors)
            top_items.push_back({ (std::string)s->Name, s->getId(), s->getResourceTypeString() });

        int choice = arrowMenu("Select a resource to rename", top_items);
        if (choice < 0)
            break;

        bool is_light = (choice < static_cast<int>(lights.size()));

        if (is_light) {
            renameLight(bridge, lights, choice);
            // Refresh display name in menu on next iteration (Name was updated optimistically)
        } else {
            renameSensor(bridge, sensors, choice - static_cast<int>(lights.size()));
        }
    }

    state_manager.stop();
    system("cls");
    std::cout << "Bye!\n";
    return 0;
}
