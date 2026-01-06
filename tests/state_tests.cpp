#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/state.h>
#include <hue4cpp/bridge.h>

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
}

TEST_CASE("StateManager light state", "[state]") {
    StateManager state_manager;
    
    SECTION("Get non-existent light state") {
        auto state = state_manager.getLightState("non-existent-id");
        REQUIRE(state.empty());
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
