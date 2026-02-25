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
            {"id", "motion-sensor-123"}, {"type", "motion"}, {"enabled", true},
            {"motion", {{"motion", true}, {"motion_valid", true}}}
        };
        MotionSensor sensor("motion-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE(sensor.getId() == "motion-sensor-123");
        REQUIRE(sensor.isEnabled());
        REQUIRE((bool)sensor.Motion == true);
        REQUIRE((bool)sensor.MotionValid == true);
    }

    SECTION("Motion sensor with no motion detected") {
        nlohmann::json sensor_json = {
            {"id", "motion-sensor-456"}, {"type", "motion"}, {"enabled", true},
            {"motion", {{"motion", false}, {"motion_valid", true}}}
        };
        MotionSensor sensor("motion-sensor-456", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((bool)sensor.Motion == false);
        REQUIRE((bool)sensor.MotionValid == true);
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
            {"id", "temp-sensor-123"}, {"type", "temperature"}, {"enabled", true},
            {"temperature", {{"temperature", 2150}, {"temperature_valid", true}}}
        };
        TemperatureSensor sensor("temp-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((float)sensor.Temperature == Catch::Approx(21.5f).epsilon(0.01));
        REQUIRE((bool)sensor.TemperatureValid == true);
    }

    SECTION("Temperature sensor zero degrees") {
        nlohmann::json sensor_json = {
            {"id", "temp-sensor-789"}, {"type", "temperature"}, {"enabled", true},
            {"temperature", {{"temperature", 0}, {"temperature_valid", true}}}
        };
        TemperatureSensor sensor("temp-sensor-789", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((float)sensor.Temperature == Catch::Approx(0.0f).epsilon(0.01));
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
            {"id", "light-sensor-123"}, {"type", "light_level"}, {"enabled", true},
            {"light", {{"light_level", 12345}, {"light_level_valid", true}}}
        };
        LightLevelSensor sensor("light-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((uint32_t)sensor.LightLevel == 12345u);
        REQUIRE((bool)sensor.LightLevelValid == true);
    }
}

TEST_CASE("ButtonSensor construction and state", "[sensor][button]") {
    Bridge bridge;

    SECTION("Construct and get type") {
        ButtonSensor sensor("button-123", &bridge);
        REQUIRE(sensor.getId() == "button-123");
        REQUIRE(sensor.getType() == SensorType::Button);
    }

    SECTION("Initial press") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-123"}, {"type", "button"}, {"enabled", true},
            {"button", {{"last_event", "initial_press"}, {"event_sequence", 1}}},
            {"metadata", {{"control_id", 1}}}
        };
        ButtonSensor sensor("button-sensor-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((ButtonEvent)sensor.LastEvent == ButtonEvent::InitialPress);
        REQUIRE((uint32_t)sensor.EventSequence == 1u);
        REQUIRE((uint32_t)sensor.ButtonId == 1u);
    }

    SECTION("Short release") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-456"}, {"type", "button"}, {"enabled", true},
            {"button", {{"last_event", "short_release"}, {"event_sequence", 2}}}
        };
        ButtonSensor sensor("button-sensor-456", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((ButtonEvent)sensor.LastEvent == ButtonEvent::ShortRelease);
        REQUIRE((uint32_t)sensor.EventSequence == 2u);
    }

    SECTION("Long press") {
        nlohmann::json sensor_json = {
            {"id", "button-sensor-789"}, {"type", "button"}, {"enabled", true},
            {"button", {{"last_event", "long_press"}, {"event_sequence", 3}}}
        };
        ButtonSensor sensor("button-sensor-789", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((ButtonEvent)sensor.LastEvent == ButtonEvent::LongPress);
    }
}

TEST_CASE("Sensor factory from JSON", "[sensor][factory]") {
    Bridge bridge;

    SECTION("Create MotionSensor from JSON") {
        auto sensor = createSensorFromJson({{"id","motion-abc"},{"type","motion"},{"enabled",true}}, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Motion);
        REQUIRE(sensor->getId() == "motion-abc");
        REQUIRE(dynamic_cast<MotionSensor*>(sensor.get()) != nullptr);
    }

    SECTION("Create TemperatureSensor from JSON") {
        auto sensor = createSensorFromJson({{"id","temp-def"},{"type","temperature"},{"enabled",true}}, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Temperature);
        REQUIRE(dynamic_cast<TemperatureSensor*>(sensor.get()) != nullptr);
    }

    SECTION("Create LightLevelSensor from JSON") {
        auto sensor = createSensorFromJson({{"id","light-ghi"},{"type","light_level"},{"enabled",true}}, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::LightLevel);
    }

    SECTION("Create ButtonSensor from JSON") {
        auto sensor = createSensorFromJson({{"id","button-jkl"},{"type","button"},{"enabled",true}}, &bridge);
        REQUIRE(sensor != nullptr);
        REQUIRE(sensor->getType() == SensorType::Button);
    }

    SECTION("Unknown sensor type returns nullptr") {
        REQUIRE(createSensorFromJson({{"id","unk"},{"type","unknown_type"},{"enabled",true}}, &bridge) == nullptr);
    }

    SECTION("Missing type returns nullptr") {
        REQUIRE(createSensorFromJson({{"id","miss"},{"enabled",true}}, &bridge) == nullptr);
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

        REQUIRE(sensors[0]->getType() == SensorType::Motion);
        REQUIRE(sensors[1]->getType() == SensorType::Temperature);
        REQUIRE(sensors[2]->getType() == SensorType::LightLevel);
        REQUIRE(sensors[3]->getType() == SensorType::Button);
    }

    SECTION("Dynamic cast to access type-specific properties") {
        std::unique_ptr<Sensor> sensor = std::make_unique<MotionSensor>("motion-test", &bridge);
        sensor->initFromJson({{"id","motion-test"},{"type","motion"},{"enabled",true},
            {"motion",{{"motion",true},{"motion_valid",true}}}});

        auto* ms = dynamic_cast<MotionSensor*>(sensor.get());
        REQUIRE(ms != nullptr);
        REQUIRE((bool)ms->Motion == true);
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
            {"id", "camera-motion-123"}, {"type", "camera_motion"}, {"enabled", true},
            {"motion", {{"motion", true}, {"motion_valid", true}}}
        };
        CameraMotionSensor sensor("camera-motion-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE(sensor.isEnabled());
        REQUIRE((bool)sensor.CameraMotion == true);
        REQUIRE((bool)sensor.CameraMotionValid == true);
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
            {"id", "bell-button-123"}, {"type", "bell_button"}, {"enabled", true},
            {"button", {{"last_event", "initial_press"}, {"event_sequence", 5}}}
        };
        BellButtonSensor sensor("bell-button-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((ButtonEvent)sensor.LastEvent == ButtonEvent::InitialPress);
        REQUIRE((uint32_t)sensor.EventSequence == 5u);
    }
}

TEST_CASE("RelativeRotarySensor construction and state", "[sensor][rotary]") {
    Bridge bridge;

    SECTION("Construct and get type") {
        RelativeRotarySensor sensor("rotary-123", &bridge);
        REQUIRE(sensor.getId() == "rotary-123");
        REQUIRE(sensor.getType() == SensorType::RelativeRotary);
    }

    SECTION("Clockwise rotation") {
        nlohmann::json sensor_json = {
            {"id","rotary-123"},{"type","relative_rotary"},{"enabled",true},
            {"relative_rotary",{
                {"last_event",{{"rotation",5},{"direction","clock_wise"},{"action","short_release"}}},
                {"event_sequence",10}
            }}
        };
        RelativeRotarySensor sensor("rotary-123", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((int32_t)sensor.Steps == 5);
        REQUIRE((RotationDirection)sensor.Direction == RotationDirection::ClockWise);
        REQUIRE((ButtonEvent)sensor.Action == ButtonEvent::ShortRelease);
        REQUIRE((uint32_t)sensor.EventSequence == 10u);
    }

    SECTION("Counter-clockwise rotation") {
        nlohmann::json sensor_json = {
            {"id","rotary-456"},{"type","relative_rotary"},{"enabled",true},
            {"relative_rotary",{
                {"last_event",{{"rotation",-3},{"direction","counter_clock_wise"},{"action","initial_press"}}},
                {"event_sequence",11}
            }}
        };
        RelativeRotarySensor sensor("rotary-456", &bridge);
        sensor.initFromJson(sensor_json);

        REQUIRE((int32_t)sensor.Steps == -3);
        REQUIRE((RotationDirection)sensor.Direction == RotationDirection::CounterClockWise);
        REQUIRE((ButtonEvent)sensor.Action == ButtonEvent::InitialPress);
    }
}

TEST_CASE("GeolocationSensor construction and state", "[sensor][geolocation]") {
    Bridge bridge;

    SECTION("Construct and get type") {
        GeolocationSensor sensor("geo-123", &bridge);
        REQUIRE(sensor.getId() == "geo-123");
        REQUIRE(sensor.getType() == SensorType::Geolocation);
    }

    SECTION("Configured") {
        nlohmann::json sensor_json = {
            {"id","geo-123"},{"type","geolocation"},{"enabled",true},{"is_configured",true}
        };
        GeolocationSensor sensor("geo-123", &bridge);
        sensor.initFromJson(sensor_json);
        REQUIRE((bool)sensor.IsConfigured == true);
    }

    SECTION("Not configured") {
        nlohmann::json sensor_json = {
            {"id","geo-456"},{"type","geolocation"},{"enabled",true},{"is_configured",false}
        };
        GeolocationSensor sensor("geo-456", &bridge);
        sensor.initFromJson(sensor_json);
        REQUIRE((bool)sensor.IsConfigured == false);
    }
}

TEST_CASE("TamperSensor construction and state", "[sensor][tamper]") {
    Bridge bridge;

    SECTION("Construct and get type") {
        TamperSensor sensor("tamper-123", &bridge);
        REQUIRE(sensor.getId() == "tamper-123");
        REQUIRE(sensor.getType() == SensorType::Tamper);
    }

    SECTION("Tampered") {
        nlohmann::json sensor_json = {
            {"id","tamper-123"},{"type","tamper"},{"enabled",true},
            {"tamper",{{"tampered",true},{"tamper_valid",true}}}
        };
        TamperSensor sensor("tamper-123", &bridge);
        sensor.initFromJson(sensor_json);
        REQUIRE((bool)sensor.Tampered == true);
        REQUIRE((bool)sensor.TamperValid == true);
    }

    SECTION("Not tampered") {
        nlohmann::json sensor_json = {
            {"id","tamper-456"},{"type","tamper"},{"enabled",true},
            {"tamper",{{"tampered",false},{"tamper_valid",true}}}
        };
        TamperSensor sensor("tamper-456", &bridge);
        sensor.initFromJson(sensor_json);
        REQUIRE((bool)sensor.Tampered == false);
        REQUIRE((bool)sensor.TamperValid == true);
    }
}

TEST_CASE("New sensor types in factory", "[sensor][factory]") {
    Bridge bridge;

    SECTION("CameraMotionSensor") {
        auto s = createSensorFromJson({{"id","cam-abc"},{"type","camera_motion"},{"enabled",true}}, &bridge);
        REQUIRE(s != nullptr);
        REQUIRE(s->getType() == SensorType::CameraMotion);
        REQUIRE(dynamic_cast<CameraMotionSensor*>(s.get()) != nullptr);
    }
    SECTION("BellButtonSensor") {
        auto s = createSensorFromJson({{"id","bell-def"},{"type","bell_button"},{"enabled",true}}, &bridge);
        REQUIRE(s != nullptr);
        REQUIRE(s->getType() == SensorType::BellButton);
    }
    SECTION("RelativeRotarySensor") {
        auto s = createSensorFromJson({{"id","rot-ghi"},{"type","relative_rotary"},{"enabled",true}}, &bridge);
        REQUIRE(s != nullptr);
        REQUIRE(s->getType() == SensorType::RelativeRotary);
    }
    SECTION("GeolocationSensor") {
        auto s = createSensorFromJson({{"id","geo-jkl"},{"type","geolocation"},{"enabled",true}}, &bridge);
        REQUIRE(s != nullptr);
        REQUIRE(s->getType() == SensorType::Geolocation);
    }
    SECTION("TamperSensor") {
        auto s = createSensorFromJson({{"id","tam-mno"},{"type","tamper"},{"enabled",true}}, &bridge);
        REQUIRE(s != nullptr);
        REQUIRE(s->getType() == SensorType::Tamper);
    }
}
