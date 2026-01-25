#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <hue4cpp/light.h>
#include <hue4cpp/bridge.h>
#include <nlohmann/json.hpp>

using namespace hue4cpp;

TEST_CASE("Light construction", "[light]") {
    SECTION("Default constructor") {
        Light light;
        REQUIRE(light.getId().empty());
        REQUIRE_FALSE(light.isOn());
    }
    
    SECTION("Constructor with ID and bridge") {
        Bridge bridge;
        Light light("light-id-123", &bridge);
        REQUIRE(light.getId() == "light-id-123");
        REQUIRE_FALSE(light.isOn());
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
    
    SECTION("Default brightness is not set") {
        REQUIRE_FALSE(light.getBrightness().has_value());
    }
    
    SECTION("Default color is not set") {
        REQUIRE_FALSE(light.getColor().has_value());
    }
    
    SECTION("Default color temperature is not set") {
        REQUIRE_FALSE(light.getColorTemperature().has_value());
    }
}

TEST_CASE("Light copy and move", "[light]") {
    SECTION("Light can be copied") {
        Bridge bridge;
        Light light1("test-light", &bridge);
        Light light2 = light1;
        REQUIRE(light2.getId() == "test-light");
    }
    
    SECTION("Light can be moved") {
        Bridge bridge;
        Light light1("test-light", &bridge);
        Light light2 = std::move(light1);
        REQUIRE(light2.getId() == "test-light");
    }
}

TEST_CASE("Light updateFromJson - basic metadata", "[light][json]") {
    Light light;
    
    SECTION("Parse light ID and name") {
        nlohmann::json light_json = {
            {"id", "12345678-1234-1234-1234-123456789abc"},
            {"metadata", {
                {"name", "Living Room Light"}
            }}
        };
        
        light.updateFromJson(light_json);
        REQUIRE(light.getId() == "12345678-1234-1234-1234-123456789abc");
        REQUIRE(light.getName() == "Living Room Light");
    }
}

TEST_CASE("Light updateFromJson - on/off state", "[light][json]") {
    Light light;
    
    SECTION("Light is on") {
        nlohmann::json light_json = {
            {"id", "test-id"},
            {"on", {{"on", true}}}
        };
        
        light.updateFromJson(light_json);
        REQUIRE(light.isOn());
    }
    
    SECTION("Light is off") {
        nlohmann::json light_json = {
            {"id", "test-id"},
            {"on", {{"on", false}}}
        };
        
        light.updateFromJson(light_json);
        REQUIRE_FALSE(light.isOn());
    }
}

TEST_CASE("Light updateFromJson - brightness", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"dimming", {{"brightness", 75.5}}}
    };
    
    light.updateFromJson(light_json);
    
    REQUIRE(light.getBrightness().has_value());
    REQUIRE(light.getBrightness().value() == 75);
    REQUIRE(light.getCapabilities().brightness);
}

TEST_CASE("Light updateFromJson - color XY", "[light][json]") {
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
    
    light.updateFromJson(light_json);
    
    REQUIRE(light.getColor().has_value());
    REQUIRE(light.getColor()->x == Catch::Approx(0.3127f).epsilon(0.0001));
    REQUIRE(light.getColor()->y == Catch::Approx(0.3290f).epsilon(0.0001));
    REQUIRE(light.getCapabilities().color);
}

TEST_CASE("Light updateFromJson - color temperature", "[light][json]") {
    Light light;
    
    nlohmann::json light_json = {
        {"id", "test-id"},
        {"color_temperature", {
            {"mirek", 366}
        }}
    };
    
    light.updateFromJson(light_json);
    
    REQUIRE(light.getColorTemperature().has_value());
    REQUIRE(light.getColorTemperature()->mireds == 366);
    REQUIRE(light.getCapabilities().color_temperature);
}

TEST_CASE("Light updateFromJson - complete light data", "[light][json]") {
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
    
    light.updateFromJson(light_json);
    
    REQUIRE(light.getId() == "abcd-1234-efgh-5678");
    REQUIRE(light.getName() == "Desk Lamp");
    REQUIRE(light.isOn());
    REQUIRE(light.getBrightness() == 50);
    REQUIRE(light.getColor().has_value());
    REQUIRE(light.getColorTemperature().has_value());
    
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
        auto result = light.turnOn();
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::AuthenticationRequired);
    }
    
    SECTION("Set brightness fails without bridge") {
        auto result = light.setBrightness(50);
        REQUIRE_FALSE(result.isSuccess());
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
    light.updateFromJson(light_json);
    
    SECTION("Brightness out of range") {
        auto result = light.setBrightness(150);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("XY color out of range") {
        XYColor invalid_color(1.5f, 0.5f);
        auto result = light.setColor(invalid_color);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("Color temperature out of range - too low") {
        ColorTemperature temp(100);
        auto result = light.setColorTemperature(temp);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
    }
    
    SECTION("Color temperature out of range - too high") {
        ColorTemperature temp(600);
        auto result = light.setColorTemperature(temp);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidParameter);
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
    light.updateFromJson(light_json);
    
    SECTION("Brightness control on non-dimmable light") {
        auto result = light.setBrightness(50);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidRequest);
    }
    
    SECTION("Color control on non-color light") {
        XYColor color(0.5f, 0.5f);
        auto result = light.setColor(color);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidRequest);
    }
    
    SECTION("Color temperature control on non-CT light") {
        ColorTemperature temp(250);
        auto result = light.setColorTemperature(temp);
        REQUIRE_FALSE(result.isSuccess());
        REQUIRE(result.error == ErrorCode::InvalidRequest);
    }
}
