#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <hue4cpp/sensor.h>
#include <hue4cpp/bridge.h>
#include <nlohmann/json.hpp>

using namespace hue4cpp;

TEST_CASE("Sensor construction", "[sensor]") {
    SECTION("Default constructor") {
        Sensor sensor;
        REQUIRE(sensor.getId().empty());
        REQUIRE(sensor.getType() == SensorType::Unknown);
        REQUIRE_FALSE(sensor.isEnabled());
    }
    
    SECTION("Constructor with ID and bridge") {
        Bridge bridge;
        Sensor sensor("sensor-id-123", &bridge);
        REQUIRE(sensor.getId() == "sensor-id-123");
        REQUIRE(sensor.getType() == SensorType::Unknown);
    }
}

TEST_CASE("Sensor copy and move", "[sensor]") {
    SECTION("Sensor can be copied") {
        Bridge bridge;
        Sensor sensor1("test-sensor", &bridge);
        Sensor sensor2 = sensor1;
        REQUIRE(sensor2.getId() == "test-sensor");
    }
    
    SECTION("Sensor can be moved") {
        Bridge bridge;
        Sensor sensor1("test-sensor", &bridge);
        Sensor sensor2 = std::move(sensor1);
        REQUIRE(sensor2.getId() == "test-sensor");
    }
}

TEST_CASE("Sensor updateFromJson - Motion sensor", "[sensor][json]") {
    Sensor sensor;
    
    SECTION("Parse motion sensor data") {
        nlohmann::json sensor_json = {
            {"id", "motion-sensor-123"},
            {"type", "motion"},
            {"enabled", true},
            {"motion", {
                {"motion", true},
                {"motion_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        REQUIRE(sensor.getId() == "motion-sensor-123");
        REQUIRE(sensor.getType() == SensorType::Motion);
        REQUIRE(sensor.isEnabled());
        
        auto motion_state = sensor.getMotionState();
        REQUIRE(motion_state.has_value());
        REQUIRE(motion_state->motion == true);
        REQUIRE(motion_state->motion_valid == true);
    }
    
    SECTION("Motion sensor with no motion detected") {
        nlohmann::json sensor_json = {
            {"id", "motion-sensor-456"},
            {"type", "motion"},
            {"enabled", true},
            {"motion", {
                {"motion", false},
                {"motion_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto motion_state = sensor.getMotionState();
        REQUIRE(motion_state.has_value());
        REQUIRE(motion_state->motion == false);
        REQUIRE(motion_state->motion_valid == true);
    }
    
    SECTION("Motion sensor disabled") {
        nlohmann::json sensor_json = {
            {"id", "motion-sensor-789"},
            {"type", "motion"},
            {"enabled", false},
            {"motion", {
                {"motion", false},
                {"motion_valid", false}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        REQUIRE_FALSE(sensor.isEnabled());
    }
}

TEST_CASE("Sensor updateFromJson - Temperature sensor", "[sensor][json]") {
    Sensor sensor;
    
    SECTION("Parse temperature sensor data") {
        nlohmann::json sensor_json = {
            {"id", "temp-sensor-123"},
            {"type", "temperature"},
            {"enabled", true},
            {"temperature", {
                {"temperature", 2150},  // 21.50°C in deci-degrees
                {"temperature_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        REQUIRE(sensor.getId() == "temp-sensor-123");
        REQUIRE(sensor.getType() == SensorType::Temperature);
        REQUIRE(sensor.isEnabled());
        
        auto temp_state = sensor.getTemperatureState();
        REQUIRE(temp_state.has_value());
        REQUIRE(temp_state->temperature == Catch::Approx(21.50f));
        REQUIRE(temp_state->temperature_valid == true);
    }
    
    SECTION("Temperature sensor with negative temperature") {
        nlohmann::json sensor_json = {
            {"id", "temp-sensor-456"},
            {"type", "temperature"},
            {"enabled", true},
            {"temperature", {
                {"temperature", -500},  // -5.00°C
                {"temperature_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto temp_state = sensor.getTemperatureState();
        REQUIRE(temp_state.has_value());
        REQUIRE(temp_state->temperature == Catch::Approx(-5.00f));
    }
}

TEST_CASE("Sensor updateFromJson - Light level sensor", "[sensor][json]") {
    Sensor sensor;
    
    SECTION("Parse light level sensor data") {
        nlohmann::json sensor_json = {
            {"id", "light-sensor-123"},
            {"type", "light_level"},
            {"enabled", true},
            {"light", {
                {"light_level", 17747},
                {"light_level_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        REQUIRE(sensor.getId() == "light-sensor-123");
        REQUIRE(sensor.getType() == SensorType::LightLevel);
        REQUIRE(sensor.isEnabled());
        
        auto light_state = sensor.getLightLevelState();
        REQUIRE(light_state.has_value());
        REQUIRE(light_state->light_level == 17747);
        REQUIRE(light_state->light_level_valid == true);
    }
    
    SECTION("Light level sensor with zero light") {
        nlohmann::json sensor_json = {
            {"id", "light-sensor-456"},
            {"type", "light_level"},
            {"enabled", true},
            {"light", {
                {"light_level", 0},
                {"light_level_valid", true}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto light_state = sensor.getLightLevelState();
        REQUIRE(light_state.has_value());
        REQUIRE(light_state->light_level == 0);
    }
}

TEST_CASE("Sensor updateFromJson - Button sensor", "[sensor][json]") {
    Sensor sensor;
    
    SECTION("Parse button sensor with short_release event") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-123"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "short_release"},
                {"event_sequence", 42}
            }},
            {"metadata", {
                {"control_id", 1}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        REQUIRE(sensor.getId() == "button-sensor-123");
        REQUIRE(sensor.getType() == SensorType::Button);
        REQUIRE(sensor.isEnabled());
        
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.has_value());
        REQUIRE(button_state->last_event == ButtonEvent::ShortRelease);
        REQUIRE(button_state->event_sequence == 42);
        REQUIRE(button_state->button_id == 1);
    }
    
    SECTION("Parse button sensor with initial_press event") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-456"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "initial_press"},
                {"event_sequence", 10}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.has_value());
        REQUIRE(button_state->last_event == ButtonEvent::InitialPress);
    }
    
    SECTION("Parse button sensor with long_release event") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-789"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "long_release"},
                {"event_sequence", 5}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.has_value());
        REQUIRE(button_state->last_event == ButtonEvent::LongRelease);
    }
    
    SECTION("Parse button sensor with unknown event") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-999"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "unknown_event_type"},
                {"event_sequence", 1}
            }}
        };
        
        sensor.updateFromJson(sensor_json);
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.has_value());
        REQUIRE(button_state->last_event == ButtonEvent::Unknown);
    }
}

TEST_CASE("Sensor state queries", "[sensor]") {
    Sensor sensor;
    
    SECTION("Motion state is not set by default") {
        REQUIRE_FALSE(sensor.getMotionState().has_value());
    }
    
    SECTION("Temperature state is not set by default") {
        REQUIRE_FALSE(sensor.getTemperatureState().has_value());
    }
    
    SECTION("Light level state is not set by default") {
        REQUIRE_FALSE(sensor.getLightLevelState().has_value());
    }
    
    SECTION("Button state is not set by default") {
        REQUIRE_FALSE(sensor.getButtonState().has_value());
    }
}

TEST_CASE("SensorType enum", "[sensor][types]") {
    SECTION("Unknown sensor type") {
        Sensor sensor;
        REQUIRE(sensor.getType() == SensorType::Unknown);
    }
    
    SECTION("Motion sensor type") {
        Sensor sensor;
        nlohmann::json json = {{"type", "motion"}};
        sensor.updateFromJson(json);
        REQUIRE(sensor.getType() == SensorType::Motion);
    }
    
    SECTION("Temperature sensor type") {
        Sensor sensor;
        nlohmann::json json = {{"type", "temperature"}};
        sensor.updateFromJson(json);
        REQUIRE(sensor.getType() == SensorType::Temperature);
    }
    
    SECTION("Light level sensor type") {
        Sensor sensor;
        nlohmann::json json = {{"type", "light_level"}};
        sensor.updateFromJson(json);
        REQUIRE(sensor.getType() == SensorType::LightLevel);
    }
    
    SECTION("Button sensor type") {
        Sensor sensor;
        nlohmann::json json = {{"type", "button"}};
        sensor.updateFromJson(json);
        REQUIRE(sensor.getType() == SensorType::Button);
    }
}

TEST_CASE("ButtonEvent enum values", "[sensor][types]") {
    SECTION("All button event types can be parsed") {
        Sensor sensor;
        
        nlohmann::json json = {
            {"type", "button"},
            {"button", {{"last_event", "initial_press"}, {"event_sequence", 1}}}
        };
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::InitialPress);
        
        json["button"]["last_event"] = "short_release";
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::ShortRelease);
        
        json["button"]["last_event"] = "long_release";
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::LongRelease);
        
        json["button"]["last_event"] = "long_press";
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::LongPress);
        
        json["button"]["last_event"] = "double_short_release";
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::DoubleShortRelease);
        
        json["button"]["last_event"] = "repeat";
        sensor.updateFromJson(json);
        REQUIRE(sensor.getButtonState()->last_event == ButtonEvent::Repeat);
    }
}
