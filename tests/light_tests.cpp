#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/light.h>
#include <hue4cpp/bridge.h>

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
