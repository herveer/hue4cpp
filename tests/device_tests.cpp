#include <catch2/catch_test_macros.hpp>
#include <hue4cpp/device.h>
#include <hue4cpp/bridge.h>
#include <nlohmann/json.hpp>

using namespace hue4cpp;

namespace {
    // A representative DeviceGet payload, mirroring the example from the
    // /resource/device API documentation.
    nlohmann::json makeDeviceJson() {
        return nlohmann::json{
            {"id", "3a5c2d9e-0000-4000-8000-000000000001"},
            {"type", "device"},
            {"product_data", {
                {"model_id", "LCB001"},
                {"manufacturer_name", "Signify Netherlands B.V."},
                {"product_name", "Hue color downlight"},
                {"product_archetype", "flood_bulb"},
                {"certified", true},
                {"software_version", "1.122.8"},
                {"hardware_platform_type", "100b-112"}
            }},
            {"metadata", {
                {"name", "Living room ceiling"},
                {"archetype", "flood_bulb"}
            }},
            {"services", nlohmann::json::array({
                {{"rid", "11111111-0000-4000-8000-000000000001"}, {"rtype", "light"}},
                {{"rid", "22222222-0000-4000-8000-000000000002"}, {"rtype", "zigbee_connectivity"}}
            })}
        };
    }
}

TEST_CASE("Device construction", "[device]") {
    Bridge bridge;

    SECTION("Default constructor leaves empty fields") {
        Device device;
        REQUIRE(device.Id == std::string(""));
        REQUIRE(device.Type == std::string(""));
        REQUIRE(device.getLights().empty());
        REQUIRE(device.getSensors().empty());
    }

    SECTION("Constructor with id and bridge") {
        Device device("device-123", &bridge);
        REQUIRE(device.Id == std::string("device-123"));
        REQUIRE(device.getLights().empty());
        REQUIRE(device.getSensors().empty());
    }
}

TEST_CASE("Device initFromJson parses metadata", "[device]") {
    Bridge bridge;
    Device device("3a5c2d9e-0000-4000-8000-000000000001", &bridge);
    device.initFromJson(makeDeviceJson());

    SECTION("Top-level fields") {
        REQUIRE(device.Id == std::string("3a5c2d9e-0000-4000-8000-000000000001"));
        REQUIRE(device.Type == std::string("device"));
        REQUIRE(device.Name == std::string("Living room ceiling"));
    }

    SECTION("Product data fields") {
        REQUIRE(device.ModelId == std::string("LCB001"));
        REQUIRE(device.ManufacturerName == std::string("Signify Netherlands B.V."));
        REQUIRE(device.ProductName == std::string("Hue color downlight"));
        REQUIRE(device.ProductArchetype == std::string("flood_bulb"));
        REQUIRE((bool)device.Certified == true);
        REQUIRE(device.SoftwareVersion == std::string("1.122.8"));
    }

    SECTION("Lists are not populated by initFromJson alone") {
        // initFromJson only parses metadata; Bridge attaches the owned
        // lights/sensors. Without that step the lists stay empty.
        REQUIRE(device.getLights().empty());
        REQUIRE(device.getSensors().empty());
    }
}

TEST_CASE("Device handles partial JSON gracefully", "[device]") {
    Bridge bridge;

    SECTION("Missing product_data leaves defaults") {
        Device device("device-min", &bridge);
        nlohmann::json json = {
            {"id", "device-min"},
            {"type", "device"}
        };
        device.initFromJson(json);

        REQUIRE(device.Id == std::string("device-min"));
        REQUIRE(device.Type == std::string("device"));
        REQUIRE(device.ModelId == std::string(""));
        REQUIRE((bool)device.Certified == false);
        REQUIRE(device.SoftwareVersion == std::string(""));
    }

    SECTION("Empty object keeps construction id") {
        Device device("device-keep", &bridge);
        device.initFromJson(nlohmann::json::object());
        REQUIRE(device.Id == std::string("device-keep"));
    }
}

TEST_CASE("Device PropertyChanged and equality guard", "[device][reactive]") {
    Bridge bridge;
    Device device("d-1", &bridge);
    device.initFromJson(makeDeviceJson());

    SECTION("PropertyChanged fires when a value changes") {
        std::string notified_property;
        device.PropertyChanged += [&](ReactiveLitepp::ObservableObject&, ReactiveLitepp::PropertyChangedArgs args) {
            notified_property = args.PropertyName();
        };

        nlohmann::json updated = makeDeviceJson();
        updated["product_data"]["software_version"] = "1.123.0";
        device.initFromJson(updated);

        REQUIRE(device.SoftwareVersion == std::string("1.123.0"));
        REQUIRE(notified_property == "SoftwareVersion");
    }

    SECTION("No notification when the same value is fed again") {
        bool notified = false;
        device.PropertyChanged += [&](ReactiveLitepp::ObservableObject&, ReactiveLitepp::PropertyChangedArgs) {
            notified = true;
        };

        device.initFromJson(makeDeviceJson());

        REQUIRE_FALSE(notified);
    }
}
