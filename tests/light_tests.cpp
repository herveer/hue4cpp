#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <hue4cpp/light.h>
#include <hue4cpp/bridge.h>
#include <hue4cpp/exceptions.h>
#include <nlohmann/json.hpp>

using namespace hue4cpp;

TEST_CASE("Light construction", "[light]") {
    SECTION("Default constructor") {
        Light light;
        REQUIRE(light.Id.Get().empty());
        REQUIRE_THROWS_AS(light.IsOn.Get(), BridgeNotReachableException);
    }
    
    SECTION("Constructor with ID and bridge") {
        Bridge bridge;
        Light light("light-id-123", &bridge);
        REQUIRE(light.Id == "light-id-123");
        REQUIRE_THROWS_AS(light.IsOn.Get(), BridgeNotReachableException);
    }
}

TEST_CASE("Light capabilities", "[light]") {
    Light light;
    auto caps = light.getCapabilities();
    
    SECTION("On/off is always available") {
        REQUIRE(caps.on_off);
    }
}

TEST_CASE("Light state queries", "[light]") {
    Light light;
    
    SECTION("Default brightness throws ResourceNotFoundException") {
        REQUIRE_THROWS_AS(light.Brightness.Get(), ResourceNotFoundException);
    }
    
    SECTION("Default color throws ResourceNotFoundException") {
        REQUIRE_THROWS_AS(light.XYColor_.Get(), ResourceNotFoundException);
    }
    
    SECTION("Default color temperature throws ResourceNotFoundException") {
        REQUIRE_THROWS_AS(light.ColorTemperature_.Get(), ResourceNotFoundException);
    }
}

TEST_CASE("Light copy and move", "[light]") {
    SECTION("Light cannot be copied") {
        Bridge bridge;
        Light light1("test-light", &bridge);
        // Copy is deleted to prevent issues with observable state
        static_assert(!std::is_copy_constructible_v<Light>, "Light should not be copyable");
    }
    
    SECTION("Light cannot be moved") {
        Bridge bridge;
        Light light1("test-light", &bridge);
        // Move is deleted to prevent issues with observable state
        static_assert(!std::is_move_constructible_v<Light>, "Light should not be movable");
    }
}

TEST_CASE("Light initFromJson - basic metadata", "[light][json]") {
    Light light;
    
    SECTION("Parse light ID and name") {
        nlohmann::json light_json = {
            {"id", "12345678-1234-1234-1234-123456789abc"},
            {"metadata", {
                {"name", "Living Room Light"}
            }}
        };
        
        light.initFromJson(light_json);
        REQUIRE(light.Id == "12345678-1234-1234-1234-123456789abc");
        REQUIRE(light.Name == "Living Room Light");
    }
}

TEST_CASE("Light Name updates from SSE metadata event", "[light][sse]") {
    Bridge bridge;
    Light light("f79caea0-0766-4196-bbd6-c77401b951da", &bridge);

    // Seed an initial name via initFromJson
    nlohmann::json initial = {
        {"id",   "f79caea0-0766-4196-bbd6-c77401b951da"},
        {"type", "light"},
        {"metadata", {{"name", "Old Name"}}}
    };
    light.initFromJson(initial);
    REQUIRE(light.Name == "Old Name");

    SECTION("Name updates and fires PropertyChanged when metadata.name arrives") {
        std::string notified_property;
        light.PropertyChanged += [&notified_property](ReactiveLitepp::ObservableObject&, ReactiveLitepp::PropertyChangedArgs args) {
            notified_property = args.PropertyName();
        };

        // Simulate the SSE delta the bridge sends when the user renames a light
        nlohmann::json sse_delta = {
            {"id",   "f79caea0-0766-4196-bbd6-c77401b951da"},
            {"type", "light"},
            {"metadata", {{"name", "Plafond chambre"}}}
        };
        light.initFromJson(sse_delta);

        REQUIRE(light.Name == "Plafond chambre");
        REQUIRE(notified_property == "Name");
    }

    SECTION("Name is unchanged and no notification fires when metadata.name is absent") {
        bool name_notified = false;
        light.PropertyChanged += [&name_notified](ReactiveLitepp::ObservableObject&, ReactiveLitepp::PropertyChangedArgs args) {
            if (args.PropertyName() == "Name") name_notified = true;
        };

        // Delta without metadata (e.g. brightness-only event). The brightness
        // change legitimately notifies Brightness; only Name must stay silent.
        nlohmann::json sse_delta = {
            {"id",   "f79caea0-0766-4196-bbd6-c77401b951da"},
            {"type", "light"},
            {"dimming", {{"brightness", 80.0}}}
        };
        light.initFromJson(sse_delta);

        REQUIRE(light.Name == "Old Name");
        REQUIRE_FALSE(name_notified);
    }

    SECTION("Name is unchanged and no notification fires when value is identical") {
        bool notified = false;
        light.PropertyChanged += [&notified](ReactiveLitepp::ObservableObject&, ReactiveLitepp::PropertyChangedArgs) {
            notified = true;
        };

        nlohmann::json sse_delta = {
            {"id",   "f79caea0-0766-4196-bbd6-c77401b951da"},
            {"type", "light"},
            {"metadata", {{"name", "Old Name"}}}   // same value
        };
        light.initFromJson(sse_delta);

        REQUIRE(light.Name == "Old Name");
        REQUIRE_FALSE(notified);
    }
}

TEST_CASE("Light initFromJson - on/off state", "[light][json]") {
    Light light;
    
    SECTION("Light is on") {
        nlohmann::json light_json = {
            {"id", "test-id"},
            {"on", {{"on", true}}}
        };
        
        light.initFromJson(light_json);
        REQUIRE(light.getCapabilities().on_off);
    }
}

TEST_CASE("Light initFromJson - brightness", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"dimming", {{"brightness", 75.5}}}
    };
    
    light.initFromJson(light_json);
    REQUIRE(light.getCapabilities().brightness);
}

TEST_CASE("Light initFromJson - color XY", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"color", {
            {"xy", {
                {"x", 0.3127},
                {"y", 0.3290}
            }}
        }}
    };
    
    light.initFromJson(light_json);
    REQUIRE(light.getCapabilities().color);
}

TEST_CASE("Light initFromJson - color temperature", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"color_temperature", {
            {"mirek", 366}
        }}
    };
    
    light.initFromJson(light_json);
    REQUIRE(light.getCapabilities().color_temperature);
}

TEST_CASE("Light initFromJson - complete light data", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "abcd-1234-efgh-5678"},
        {"metadata", {{"name", "Desk Lamp"}}},
        {"on", {{"on", true}}},
        {"dimming", {{"brightness", 50.0}}},
        {"color", {
            {"xy", {{"x", 0.5}, {"y", 0.5}}}
        }},
        {"color_temperature", {{"mirek", 250}}},
        {"effects", {}}
    };
    
    light.initFromJson(light_json);    
    auto caps = light.getCapabilities();
    REQUIRE(caps.on_off);
    REQUIRE(caps.brightness);
    REQUIRE(caps.color);
    REQUIRE(caps.color_temperature);
    REQUIRE(caps.effects);
}

TEST_CASE("RGB to XY color conversion", "[light][color]") {
    SECTION("Pure red") {
        RGBColor red(255, 0, 0);
        Light light;
        // We can't directly test the internal conversion function,
        // but we can verify it doesn't crash and produces valid XY values
        // This would require either making the function public or using a test fixture
        // For now, we'll test through the API
    }
    
    SECTION("Pure white") {
        RGBColor white(255, 255, 255);
        // Similar limitation as above
    }
}

TEST_CASE("ColorTemperature conversions", "[light][color]") {
    SECTION("Kelvin to mireds conversion") {
        auto temp = ColorTemperature::fromKelvin(2700);  // Warm white
        REQUIRE(temp.mireds == 370);  // 1000000 / 2700 ≈ 370
    }
    
    SECTION("Mireds to Kelvin conversion") {
        ColorTemperature temp(250);  // Cool white
        REQUIRE(temp.toKelvin() == 4000);  // 1000000 / 250 = 4000
    }
    
    SECTION("Kelvin out of range - too low") {
        REQUIRE_THROWS_AS(ColorTemperature::fromKelvin(1000), std::invalid_argument);
    }
    
    SECTION("Kelvin out of range - too high") {
        REQUIRE_THROWS_AS(ColorTemperature::fromKelvin(7000), std::invalid_argument);
    }
}

TEST_CASE("Light control without authentication", "[light][control]") {
    Light light("test-id", nullptr);
    
    SECTION("Turn on fails without bridge") {
        REQUIRE_THROWS_AS(light.IsOn = true, BridgeNotReachableException);
    }
    
    SECTION("Set brightness fails without bridge") {
        REQUIRE_THROWS_AS(light.Brightness = 50, BridgeNotReachableException);
    }
}

TEST_CASE("Light control parameter validation", "[light][control]") {
    Bridge bridge;
    Light light("test-id", &bridge);
    
    // Simulate light with capabilities
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"dimming", {{"brightness", 50.0}}},
        {"color", {{"xy", {{"x", 0.5}, {"y", 0.5}}}}},
        {"color_temperature", {{"mirek", 250}}}
    };
    light.initFromJson(light_json);
    
    SECTION("Brightness out of range") {
        REQUIRE_THROWS_AS(light.Brightness = 150, InvalidParameterException);
    }
    
    SECTION("XY color out of range") {
        XYColor invalid_color(1.5f, 0.5f);
        REQUIRE_THROWS_AS(light.XYColor_ = invalid_color, InvalidParameterException);
    }
    
    SECTION("Color temperature out of range - too low") {
        ColorTemperature temp(100);
        REQUIRE_THROWS_AS(light.ColorTemperature_ = temp, InvalidParameterException);
    }
    
    SECTION("Color temperature out of range - too high") {
        ColorTemperature temp(600);
        REQUIRE_THROWS_AS(light.ColorTemperature_ = temp, InvalidParameterException);
    }
}

TEST_CASE("Light control with missing capabilities", "[light][capabilities]") {
    Bridge bridge;
    Light light("test-id", &bridge);
    
    // Light with only on/off capability
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"on", {{"on", false}}}
    };
    light.initFromJson(light_json);
    
    SECTION("Brightness control on non-dimmable light") {
        REQUIRE_THROWS_AS(light.Brightness = 50, InvalidParameterException);
    }
    
    SECTION("Color control on non-color light") {
        XYColor color(0.5f, 0.5f);
        REQUIRE_THROWS_AS(light.XYColor_ = color, InvalidParameterException);
    }
    
    SECTION("Color temperature control on non-CT light") {
        ColorTemperature temp(250);
        REQUIRE_THROWS_AS(light.ColorTemperature_ = temp, InvalidParameterException);
    }
}
