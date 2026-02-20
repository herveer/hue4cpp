#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <hue4cpp/sensors.h>
#include <hue4cpp/bridge.h>
#include <nlohmann/json.hpp>

using namespace hue4cpp;

TEST_CASE("MotionSensor construction and state", "[sensor][motion]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        MotionSensor sensor("motion-123", &bridge);
        REQUIRE(sensor.getId() == "motion-123");
        REQUIRE(sensor.getType() == SensorType::Motion);
    }
    
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
        
        MotionSensor sensor("motion-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        REQUIRE(sensor.getId() == "motion-sensor-123");
        REQUIRE(sensor.getType() == SensorType::Motion);
        REQUIRE(sensor.isEnabled());
        
        auto motion_state = sensor.getMotionState();
        REQUIRE(motion_state.motion == true);
        REQUIRE(motion_state.motion_valid == true);
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
        
        MotionSensor sensor("motion-sensor-456", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto motion_state = sensor.getMotionState();
        REQUIRE(motion_state.motion == false);
        REQUIRE(motion_state.motion_valid == true);
    }
}

TEST_CASE("TemperatureSensor construction and state", "[sensor][temperature]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        TemperatureSensor sensor("temp-123", &bridge);
        REQUIRE(sensor.getId() == "temp-123");
        REQUIRE(sensor.getType() == SensorType::Temperature);
    }
    
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
        
        TemperatureSensor sensor("temp-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto temp_state = sensor.getTemperatureState();
        REQUIRE(temp_state.temperature == Catch::Approx(21.5f).epsilon(0.01));
        REQUIRE(temp_state.temperature_valid == true);
    }
    
    SECTION("Temperature sensor conversion") {
        nlohmann::json sensor_json = {
            {"id", "temp-sensor-789"},
            {"type", "temperature"},
            {"enabled", true},
            {"temperature", {
                {"temperature", 0},  // 0°C
                {"temperature_valid", true}
            }}
        };
        
        TemperatureSensor sensor("temp-sensor-789", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto temp_state = sensor.getTemperatureState();
        REQUIRE(temp_state.temperature == Catch::Approx(0.0f).epsilon(0.01));
    }
}

TEST_CASE("LightLevelSensor construction and state", "[sensor][lightlevel]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        LightLevelSensor sensor("light-123", &bridge);
        REQUIRE(sensor.getId() == "light-123");
        REQUIRE(sensor.getType() == SensorType::LightLevel);
    }
    
    SECTION("Parse light level sensor data") {
        nlohmann::json sensor_json = {
            {"id", "light-sensor-123"},
            {"type", "light_level"},
            {"enabled", true},
            {"light", {
                {"light_level", 12345},
                {"light_level_valid", true}
            }}
        };
        
        LightLevelSensor sensor("light-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto light_state = sensor.getLightLevelState();
        REQUIRE(light_state.light_level == 12345);
        REQUIRE(light_state.light_level_valid == true);
    }
}

TEST_CASE("ButtonSensor construction and state", "[sensor][button]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        ButtonSensor sensor("button-123", &bridge);
        REQUIRE(sensor.getId() == "button-123");
        REQUIRE(sensor.getType() == SensorType::Button);
    }
    
    SECTION("Parse button sensor data - initial press") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-123"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "initial_press"},
                {"event_sequence", 1}
            }},
            {"metadata", {
                {"control_id", 1}
            }}
        };
        
        ButtonSensor sensor("button-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.last_event == ButtonEvent::InitialPress);
        REQUIRE(button_state.event_sequence == 1);
        REQUIRE(button_state.button_id == 1);
    }
    
    SECTION("Parse button sensor data - short release") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-456"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "short_release"},
                {"event_sequence", 2}
            }}
        };
        
        ButtonSensor sensor("button-sensor-456", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.last_event == ButtonEvent::ShortRelease);
        REQUIRE(button_state.event_sequence == 2);
    }
    
    SECTION("Parse button sensor data - long press") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-789"},
            {"type", "button"},
            {"enabled", true},
            {"button", {
                {"last_event", "long_press"},
                {"event_sequence", 3}
            }}
        };
        
        ButtonSensor sensor("button-sensor-789", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto button_state = sensor.getButtonState();
        REQUIRE(button_state.last_event == ButtonEvent::LongPress);
    }
}

TEST_CASE("Sensor factory from JSON", "[sensor][factory]") {
    Bridge bridge;
    
    SECTION("Create MotionSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "motion-abc"},
            {"type", "motion"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Motion);
        REQUIRE(sensor->getId() == "motion-abc");
        
        // Can downcast to MotionSensor
        auto* motion_sensor = dynamic_cast<MotionSensor*>(sensor.get());
        REQUIRE(motion_sensor != nullptr);
    }
    
    SECTION("Create TemperatureSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "temp-def"},
            {"type", "temperature"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Temperature);
        
        auto* temp_sensor = dynamic_cast<TemperatureSensor*>(sensor.get());
        REQUIRE(temp_sensor != nullptr);
    }
    
    SECTION("Create LightLevelSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "light-ghi"},
            {"type", "light_level"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::LightLevel);
    }
    
    SECTION("Create ButtonSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "button-jkl"},
            {"type", "button"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Button);
    }
    
    SECTION("Unknown sensor type returns nullptr") {
        nlohmann::json sensor_json = {
            {"id", "unknown-mno"},
            {"type", "unknown_type"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor == nullptr);
    }
    
    SECTION("Missing type returns nullptr") {
        nlohmann::json sensor_json = {
            {"id", "missing-pqr"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor == nullptr);
    }
}

TEST_CASE("Polymorphic sensor usage", "[sensor][polymorphism]") {
    Bridge bridge;
    
    SECTION("Store different sensor types in base pointer vector") {
        std::vector<std::unique_ptr<Sensor>> sensors;
        
        sensors.push_back(std::make_unique<MotionSensor>("motion-1", &bridge));
        sensors.push_back(std::make_unique<TemperatureSensor>("temp-1", &bridge));
        sensors.push_back(std::make_unique<LightLevelSensor>("light-1", &bridge));
        sensors.push_back(std::make_unique<ButtonSensor>("button-1", &bridge));
        
        REQUIRE(sensors.size() == 4);
        REQUIRE(sensors[0]->getType() == SensorType::Motion);
        REQUIRE(sensors[1]->getType() == SensorType::Temperature);
        REQUIRE(sensors[2]->getType() == SensorType::LightLevel);
        REQUIRE(sensors[3]->getType() == SensorType::Button);
    }
    
    SECTION("Dynamic cast to access type-specific methods") {
        std::unique_ptr<Sensor> sensor = std::make_unique<MotionSensor>("motion-test", &bridge);
        
        // Update with data
        nlohmann::json sensor_json = {
            {"id", "motion-test"},
            {"type", "motion"},
            {"enabled", true},
            {"motion", {
                {"motion", true},
                {"motion_valid", true}
            }}
        };
        sensor->initFromJson(sensor_json);
        
        // Cast to specific type
        if (auto* motion_sensor = dynamic_cast<MotionSensor*>(sensor.get())) {
            auto state = motion_sensor->getMotionState();
            REQUIRE(state.motion == true);
        } else {
            FAIL("Dynamic cast failed");
        }
    }
}

TEST_CASE("CameraMotionSensor construction and state", "[sensor][cameramotion]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        CameraMotionSensor sensor("camera-motion-123", &bridge);
        REQUIRE(sensor.getId() == "camera-motion-123");
        REQUIRE(sensor.getType() == SensorType::CameraMotion);
    }
    
    SECTION("Parse camera motion sensor data") {
        nlohmann::json sensor_json = {
            {"id", "camera-motion-123"},
            {"type", "camera_motion"},
            {"enabled", true},
            {"motion", {
                {"motion", true},
                {"motion_valid", true}
            }}
        };
        
        CameraMotionSensor sensor("camera-motion-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        REQUIRE(sensor.getId() == "camera-motion-123");
        REQUIRE(sensor.getType() == SensorType::CameraMotion);
        REQUIRE(sensor.isEnabled());
        
        auto camera_motion_state = sensor.getCameraMotionState();
        REQUIRE(camera_motion_state.motion == true);
        REQUIRE(camera_motion_state.motion_valid == true);
    }
}

TEST_CASE("BellButtonSensor construction and state", "[sensor][bellbutton]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        BellButtonSensor sensor("bell-123", &bridge);
        REQUIRE(sensor.getId() == "bell-123");
        REQUIRE(sensor.getType() == SensorType::BellButton);
    }
    
    SECTION("Parse bell button sensor data") {
        nlohmann::json sensor_json = {
            {"id", "bell-button-123"},
            {"type", "bell_button"},
            {"enabled", true},
            {"button", {
                {"last_event", "initial_press"},
                {"event_sequence", 5}
            }}
        };
        
        BellButtonSensor sensor("bell-button-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto bell_state = sensor.getBellButtonState();
        REQUIRE(bell_state.last_event == ButtonEvent::InitialPress);
        REQUIRE(bell_state.event_sequence == 5);
    }
}

TEST_CASE("RelativeRotarySensor construction and state", "[sensor][rotary]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        RelativeRotarySensor sensor("rotary-123", &bridge);
        REQUIRE(sensor.getId() == "rotary-123");
        REQUIRE(sensor.getType() == SensorType::RelativeRotary);
    }
    
    SECTION("Parse relative rotary sensor data - clockwise") {
        nlohmann::json sensor_json = {
            {"id", "rotary-123"},
            {"type", "relative_rotary"},
            {"enabled", true},
            {"relative_rotary", {
                {"last_event", {
                    {"rotation", 5},
                    {"direction", "clock_wise"},
                    {"action", "short_release"}
                }},
                {"event_sequence", 10}
            }}
        };
        
        RelativeRotarySensor sensor("rotary-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto rotary_state = sensor.getRelativeRotaryState();
        REQUIRE(rotary_state.steps == 5);
        REQUIRE(rotary_state.direction == RotationDirection::ClockWise);
        REQUIRE(rotary_state.action == ButtonEvent::ShortRelease);
        REQUIRE(rotary_state.event_sequence == 10);
    }
    
    SECTION("Parse relative rotary sensor data - counter-clockwise") {
        nlohmann::json sensor_json = {
            {"id", "rotary-456"},
            {"type", "relative_rotary"},
            {"enabled", true},
            {"relative_rotary", {
                {"last_event", {
                    {"rotation", -3},
                    {"direction", "counter_clock_wise"},
                    {"action", "initial_press"}
                }},
                {"event_sequence", 11}
            }}
        };
        
        RelativeRotarySensor sensor("rotary-456", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto rotary_state = sensor.getRelativeRotaryState();
        REQUIRE(rotary_state.steps == -3);
        REQUIRE(rotary_state.direction == RotationDirection::CounterClockWise);
        REQUIRE(rotary_state.action == ButtonEvent::InitialPress);
    }
}

TEST_CASE("GeolocationSensor construction and state", "[sensor][geolocation]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        GeolocationSensor sensor("geo-123", &bridge);
        REQUIRE(sensor.getId() == "geo-123");
        REQUIRE(sensor.getType() == SensorType::Geolocation);
    }
    
    SECTION("Parse geolocation sensor data") {
        nlohmann::json sensor_json = {
            {"id", "geo-123"},
            {"type", "geolocation"},
            {"enabled", true},
            {"is_configured", true}
        };
        
        GeolocationSensor sensor("geo-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto geo_state = sensor.getGeolocationState();
        REQUIRE(geo_state.is_configured == true);
    }
    
    SECTION("Parse unconfigured geolocation sensor") {
        nlohmann::json sensor_json = {
            {"id", "geo-456"},
            {"type", "geolocation"},
            {"enabled", true},
            {"is_configured", false}
        };
        
        GeolocationSensor sensor("geo-456", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto geo_state = sensor.getGeolocationState();
        REQUIRE(geo_state.is_configured == false);
    }
}

TEST_CASE("TamperSensor construction and state", "[sensor][tamper]") {
    Bridge bridge;
    
    SECTION("Construct and get type") {
        TamperSensor sensor("tamper-123", &bridge);
        REQUIRE(sensor.getId() == "tamper-123");
        REQUIRE(sensor.getType() == SensorType::Tamper);
    }
    
    SECTION("Parse tamper sensor data - tampered") {
        nlohmann::json sensor_json = {
            {"id", "tamper-123"},
            {"type", "tamper"},
            {"enabled", true},
            {"tamper", {
                {"tampered", true},
                {"tamper_valid", true}
            }}
        };
        
        TamperSensor sensor("tamper-123", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto tamper_state = sensor.getTamperState();
        REQUIRE(tamper_state.tampered == true);
        REQUIRE(tamper_state.tamper_valid == true);
    }
    
    SECTION("Parse tamper sensor data - not tampered") {
        nlohmann::json sensor_json = {
            {"id", "tamper-456"},
            {"type", "tamper"},
            {"enabled", true},
            {"tamper", {
                {"tampered", false},
                {"tamper_valid", true}
            }}
        };
        
        TamperSensor sensor("tamper-456", &bridge);
        sensor.initFromJson(sensor_json);
        
        auto tamper_state = sensor.getTamperState();
        REQUIRE(tamper_state.tampered == false);
        REQUIRE(tamper_state.tamper_valid == true);
    }
}

TEST_CASE("New sensor types in factory", "[sensor][factory]") {
    Bridge bridge;
    
    SECTION("Create CameraMotionSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "camera-abc"},
            {"type", "camera_motion"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::CameraMotion);
        
        auto* camera_sensor = dynamic_cast<CameraMotionSensor*>(sensor.get());
        REQUIRE(camera_sensor != nullptr);
    }
    
    SECTION("Create BellButtonSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "bell-def"},
            {"type", "bell_button"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::BellButton);
    }
    
    SECTION("Create RelativeRotarySensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "rotary-ghi"},
            {"type", "relative_rotary"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::RelativeRotary);
    }
    
    SECTION("Create GeolocationSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "geo-jkl"},
            {"type", "geolocation"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Geolocation);
    }
    
    SECTION("Create TamperSensor from JSON") {
        nlohmann::json sensor_json = {
            {"id", "tamper-mno"},
            {"type", "tamper"},
            {"enabled", true}
        };
        
        auto sensor = createSensorFromJson(sensor_json, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Tamper);
    }
}
