#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/state.h>
#include <hue4cpp/bridge.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace hue4cpp;

TEST_CASE("StateManager construction", "[state]") {
    SECTION("Default constructor") {
        StateManager state_manager;
        REQUIRE_FALSE(state_manager.isRunning());
    }
    
    SECTION("Constructor with bridge") {
        Bridge bridge;
        StateManager state_manager(&bridge);
        REQUIRE_FALSE(state_manager.isRunning());
    }
}

TEST_CASE("StateManager callbacks", "[state]") {
    StateManager state_manager;
    
    SECTION("Register and unregister callback") {
        bool callback_called = false;
        auto callback = [&callback_called](const Event& event) {
            callback_called = true;
        };
        
        auto id = state_manager.registerCallback(callback);
        REQUIRE(id > 0);
        
        state_manager.unregisterCallback(id);
        REQUIRE_FALSE(callback_called); // Not called since no events
    }
    
    SECTION("Multiple callback registration") {
        int callback1_count = 0;
        int callback2_count = 0;
        
        auto id1 = state_manager.registerCallback([&callback1_count](const Event& event) {
            callback1_count++;
        });
        
        auto id2 = state_manager.registerCallback([&callback2_count](const Event& event) {
            callback2_count++;
        });
        
        REQUIRE(id1 != id2);
        REQUIRE(id1 > 0);
        REQUIRE(id2 > 0);
        
        state_manager.unregisterCallback(id1);
        state_manager.unregisterCallback(id2);
    }
}

TEST_CASE("StateManager light state", "[state]") {
    StateManager state_manager;
    
    SECTION("Get non-existent light state") {
        auto state = state_manager.getLightState("non-existent-id");
        REQUIRE(state.empty());
    }
    
    SECTION("Update and retrieve light state") {
        // Simulate SSE event
        nlohmann::json event_json = nlohmann::json::array();
        nlohmann::json event_item = {
            {"type", "update"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-123"},
                    {"type", "light"},
                    {"on", {{"on", true}}},
                    {"dimming", {{"brightness", 75.0}}}
                }
            })}
        };
        event_json.push_back(event_item);
        
        state_manager.updateFromEvent(event_json.dump());
        
        auto state = state_manager.getLightState("light-123");
        REQUIRE_FALSE(state.empty());
        
        // Verify the state contains expected data
        auto state_json = nlohmann::json::parse(state);
        REQUIRE(state_json["id"] == "light-123");
        REQUIRE(state_json["type"] == "light");
    }
}

TEST_CASE("StateManager event processing", "[state]") {
    StateManager state_manager;
    
    SECTION("Process light state change event") {
        bool callback_called = false;
        EventType received_type = EventType::Unknown;
        std::string received_id;
        
        state_manager.registerCallback([&](const Event& event) {
            callback_called = true;
            received_type = event.type;
            received_id = event.resource_id;
        });
        
        // Create SSE event
        nlohmann::json event_json = nlohmann::json::array();
        nlohmann::json event_item = {
            {"type", "update"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-456"},
                    {"type", "light"},
                    {"on", {{"on", false}}}
                }
            })}
        };
        event_json.push_back(event_item);
        
        state_manager.updateFromEvent(event_json.dump());
        
        REQUIRE(callback_called);
        REQUIRE(received_type == EventType::LightStateChanged);
        REQUIRE(received_id == "light-456");
    }
    
    SECTION("Process light added event") {
        EventType received_type = EventType::Unknown;
        
        state_manager.registerCallback([&](const Event& event) {
            received_type = event.type;
        });
        
        nlohmann::json event_json = nlohmann::json::array();
        nlohmann::json event_item = {
            {"type", "add"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-new"},
                    {"type", "light"}
                }
            })}
        };
        event_json.push_back(event_item);
        
        state_manager.updateFromEvent(event_json.dump());
        
        REQUIRE(received_type == EventType::LightAdded);
    }
    
    SECTION("Process light removed event") {
        EventType received_type = EventType::Unknown;
        
        // First add a light
        nlohmann::json add_event = nlohmann::json::array();
        nlohmann::json add_item = {
            {"type", "add"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-to-remove"},
                    {"type", "light"},
                    {"on", {{"on", true}}}
                }
            })}
        };
        add_event.push_back(add_item);
        state_manager.updateFromEvent(add_event.dump());
        
        // Verify it's in the cache
        auto state_before = state_manager.getLightState("light-to-remove");
        REQUIRE_FALSE(state_before.empty());
        
        // Register callback to capture remove event
        state_manager.registerCallback([&](const Event& event) {
            received_type = event.type;
        });
        
        // Now delete the light
        nlohmann::json delete_event = nlohmann::json::array();
        nlohmann::json delete_item = {
            {"type", "delete"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-to-remove"},
                    {"type", "light"}
                }
            })}
        };
        delete_event.push_back(delete_item);
        
        state_manager.updateFromEvent(delete_event.dump());
        
        REQUIRE(received_type == EventType::LightRemoved);
        
        // Verify it's removed from the cache
        auto state_after = state_manager.getLightState("light-to-remove");
        REQUIRE(state_after.empty());
    }
    
    SECTION("Handle invalid JSON gracefully") {
        bool callback_called = false;
        state_manager.registerCallback([&](const Event& event) {
            callback_called = true;
        });
        
        // Invalid JSON should not crash
        state_manager.updateFromEvent("not valid json");
        state_manager.updateFromEvent("");
        state_manager.updateFromEvent("{}");
        
        REQUIRE_FALSE(callback_called);
    }
    
    SECTION("Ignore non-light resource types") {
        bool callback_called = false;
        state_manager.registerCallback([&](const Event& event) {
            callback_called = true;
        });
        
        nlohmann::json event_json = nlohmann::json::array();
        nlohmann::json event_item = {
            {"type", "update"},
            {"data", nlohmann::json::array({
                {
                    {"id", "room-123"},
                    {"type", "room"}
                }
            })}
        };
        event_json.push_back(event_item);
        
        state_manager.updateFromEvent(event_json.dump());
        
        // Should not trigger callback for non-light resources
        REQUIRE_FALSE(callback_called);
    }
}

TEST_CASE("Event structure", "[state]") {
    SECTION("Default event construction") {
        Event event;
        REQUIRE(event.type == EventType::Unknown);
        REQUIRE(event.resource_id.empty());
        REQUIRE(event.data.empty());
    }
    
    SECTION("Event with parameters") {
        Event event(EventType::LightStateChanged, "light-123", "{}");
        REQUIRE(event.type == EventType::LightStateChanged);
        REQUIRE(event.resource_id == "light-123");
        REQUIRE(event.data == "{}");
    }
}

TEST_CASE("Color types", "[types]") {
    SECTION("RGBColor construction") {
        RGBColor rgb(255.0f, 128.0f, 64.0f);
        REQUIRE(rgb.r == 255.0f);
        REQUIRE(rgb.g == 128.0f);
        REQUIRE(rgb.b == 64.0f);
    }
    
    SECTION("XYColor construction") {
        XYColor xy(0.5f, 0.3f);
        REQUIRE(xy.x == 0.5f);
        REQUIRE(xy.y == 0.3f);
    }
    
    SECTION("ColorTemperature from Kelvin") {
        auto ct = ColorTemperature::fromKelvin(2700);
        // Allow for rounding error (mireds = 1000000 / kelvin)
        REQUIRE(ct.toKelvin() >= 2698);
        REQUIRE(ct.toKelvin() <= 2702);
    }
}
