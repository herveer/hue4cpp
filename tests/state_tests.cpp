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

TEST_CASE("StateManager event subscriptions", "[state]") {
    StateManager state_manager;

    SECTION("Subscribe and unsubscribe via scoped subscription") {
        bool called = false;
        {
            auto sub = state_manager.OnResourceEvent.SubscribeScoped(
                [&called](const ResourceEventArgs& e) {
                    if (e.type == EventType::LightStateChanged) called = true;
                });
            // sub goes out of scope – subscription is automatically removed
        }
        REQUIRE_FALSE(called);
    }

    SECTION("Multiple independent subscriptions") {
        int count1 = 0, count2 = 0;

        auto sub1 = state_manager.OnResourceEvent.SubscribeScoped(
            [&count1](const ResourceEventArgs&) { count1++; });
        auto sub2 = state_manager.OnResourceEvent.SubscribeScoped(
            [&count2](const ResourceEventArgs&) { count2++; });

        REQUIRE(count1 == 0);
        REQUIRE(count2 == 0);
    }
}

TEST_CASE("StateManager light state", "[state]") {
    StateManager state_manager;

    SECTION("Get non-existent light state") {
        auto state = state_manager.getResourceState("non-existent-id");
        REQUIRE(state.empty());
    }

    SECTION("Update and retrieve light state") {
        nlohmann::json event_json = nlohmann::json::array();
        event_json.push_back({
            {"type", "update"},
            {"data", nlohmann::json::array({
                {
                    {"id", "light-123"},
                    {"type", "light"},
                    {"on", {{"on", true}}},
                    {"dimming", {{"brightness", 75.0}}}
                }
            })}
        });

        state_manager.updateFromEvent(event_json.dump());

        auto state = state_manager.getResourceState("light-123");
        REQUIRE_FALSE(state.empty());

        auto state_parsed = nlohmann::json::parse(state);
        REQUIRE(state_parsed["id"] == "light-123");
        REQUIRE(state_parsed["type"] == "light");
    }
}

TEST_CASE("StateManager event processing", "[state]") {
    StateManager state_manager;

    SECTION("Process light state change event") {
        bool called = false;
        std::string received_id;

        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs& e) {
                if (e.type == EventType::LightStateChanged) {
                    called = true;
                    received_id = e.resource_id;
                }
            });

        nlohmann::json event_json = nlohmann::json::array();
        event_json.push_back({
            {"type", "update"},
            {"data", nlohmann::json::array({
                {{"id", "light-456"}, {"type", "light"}, {"on", {{"on", false}}}}
            })}
        });

        state_manager.updateFromEvent(event_json.dump());

        REQUIRE(called);
        REQUIRE(received_id == "light-456");
    }

    SECTION("Process light added event") {
        std::string added_id;

        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs& e) {
                if (e.type == EventType::LightAdded) added_id = e.resource_id;
            });

        nlohmann::json event_json = nlohmann::json::array();
        event_json.push_back({
            {"type", "add"},
            {"data", nlohmann::json::array({
                {{"id", "light-new"}, {"type", "light"}}
            })}
        });

        state_manager.updateFromEvent(event_json.dump());

        REQUIRE(added_id == "light-new");
    }

    SECTION("Process light removed event") {
        // First add a light to the cache
        nlohmann::json add_event = nlohmann::json::array();
        add_event.push_back({
            {"type", "add"},
            {"data", nlohmann::json::array({
                {{"id", "light-to-remove"}, {"type", "light"}, {"on", {{"on", true}}}}
            })}
        });
        state_manager.updateFromEvent(add_event.dump());

        REQUIRE_FALSE(state_manager.getResourceState("light-to-remove").empty());

        std::string removed_id;
        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs& e) {
                if (e.type == EventType::LightRemoved) removed_id = e.resource_id;
            });

        nlohmann::json delete_event = nlohmann::json::array();
        delete_event.push_back({
            {"type", "delete"},
            {"data", nlohmann::json::array({
                {{"id", "light-to-remove"}, {"type", "light"}}
            })}
        });
        state_manager.updateFromEvent(delete_event.dump());

        REQUIRE(removed_id == "light-to-remove");
        REQUIRE(state_manager.getResourceState("light-to-remove").empty());
    }

    SECTION("Handle invalid JSON gracefully") {
        bool called = false;
        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs&) { called = true; });

        state_manager.updateFromEvent("not valid json");
        state_manager.updateFromEvent("");
        state_manager.updateFromEvent("{}");

        REQUIRE_FALSE(called);
    }

    SECTION("Ignore non-light, non-sensor resource types") {
        bool called = false;
        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs&) { called = true; });

        nlohmann::json event_json = nlohmann::json::array();
        event_json.push_back({
            {"type", "update"},
            {"data", nlohmann::json::array({
                {{"id", "room-123"}, {"type", "room"}}
            })}
        });

        state_manager.updateFromEvent(event_json.dump());

        REQUIRE_FALSE(called);
    }

    SECTION("Process sensor state change event") {
        bool called = false;
        EventType received_type = EventType::Unknown;

        auto sub = state_manager.OnResourceEvent.SubscribeScoped(
            [&](const ResourceEventArgs& e) {
                if (e.isSensorEvent()) {
                    called = true;
                    received_type = e.type;
                }
            });

        nlohmann::json event_json = nlohmann::json::array();
        event_json.push_back({
            {"type", "update"},
            {"data", nlohmann::json::array({
                {{"id", "sensor-789"}, {"type", "motion"}}
            })}
        });

        state_manager.updateFromEvent(event_json.dump());

        REQUIRE(called);
        REQUIRE(received_type == EventType::SensorStateChanged);
    }

    SECTION("isLightEvent / isSensorEvent / isBridgeEvent helpers") {
        REQUIRE(ResourceEventArgs(EventType::LightStateChanged, "").isLightEvent());
        REQUIRE(ResourceEventArgs(EventType::LightAdded,        "").isLightEvent());
        REQUIRE(ResourceEventArgs(EventType::LightRemoved,      "").isLightEvent());
        REQUIRE(ResourceEventArgs(EventType::SensorStateChanged,"").isSensorEvent());
        REQUIRE(ResourceEventArgs(EventType::SensorAdded,       "").isSensorEvent());
        REQUIRE(ResourceEventArgs(EventType::SensorRemoved,     "").isSensorEvent());
        REQUIRE(ResourceEventArgs(EventType::BridgeConnected,   "").isBridgeEvent());
        REQUIRE(ResourceEventArgs(EventType::BridgeDisconnected,"").isBridgeEvent());
        REQUIRE_FALSE(ResourceEventArgs(EventType::LightStateChanged,"").isSensorEvent());
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
        REQUIRE(ct.toKelvin() >= 2698);
        REQUIRE(ct.toKelvin() <= 2702);
    }
}
