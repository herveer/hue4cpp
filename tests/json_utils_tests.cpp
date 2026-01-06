#include <catch2/catch_test_macros.hpp>
#include "hue4cpp/json_utils.h"

using namespace hue4cpp;
using namespace hue4cpp::json_utils;

TEST_CASE("JSON parsing", "[json_utils]") {
    SECTION("Parse valid JSON object") {
        std::string json_str = R"({"key": "value", "number": 42})";
        auto json = parse(json_str);
        
        REQUIRE(json.is_object());
        REQUIRE(json["key"] == "value");
        REQUIRE(json["number"] == 42);
    }
    
    SECTION("Parse valid JSON array") {
        std::string json_str = R"([1, 2, 3, 4, 5])";
        auto json = parse(json_str);
        
        REQUIRE(json.is_array());
        REQUIRE(json.size() == 5);
        REQUIRE(json[0] == 1);
    }
    
    SECTION("Parse invalid JSON throws exception") {
        std::string invalid_json = "{invalid json}";
        REQUIRE_THROWS_AS(parse(invalid_json), JsonParseException);
    }
}

TEST_CASE("JSON getValue", "[json_utils]") {
    std::string json_str = R"({"name": "test", "count": 10, "active": true})";
    auto json = parse(json_str);
    
    SECTION("Get existing string value") {
        auto value = getValue<std::string>(json, "name");
        REQUIRE(value.has_value());
        REQUIRE(value.value() == "test");
    }
    
    SECTION("Get existing integer value") {
        auto value = getValue<int>(json, "count");
        REQUIRE(value.has_value());
        REQUIRE(value.value() == 10);
    }
    
    SECTION("Get existing boolean value") {
        auto value = getValue<bool>(json, "active");
        REQUIRE(value.has_value());
        REQUIRE(value.value() == true);
    }
    
    SECTION("Get non-existing value returns nullopt") {
        auto value = getValue<std::string>(json, "missing");
        REQUIRE_FALSE(value.has_value());
    }
    
    SECTION("Get value with wrong type returns nullopt") {
        auto value = getValue<std::string>(json, "count");
        REQUIRE_FALSE(value.has_value());
    }
}

TEST_CASE("JSON getValueOr", "[json_utils]") {
    std::string json_str = R"({"name": "test", "count": 10})";
    auto json = parse(json_str);
    
    SECTION("Get existing value") {
        auto value = getValueOr<std::string>(json, "name", "default");
        REQUIRE(value == "test");
    }
    
    SECTION("Get non-existing value returns default") {
        auto value = getValueOr<std::string>(json, "missing", "default");
        REQUIRE(value == "default");
    }
    
    SECTION("Get value with wrong type returns default") {
        auto value = getValueOr<std::string>(json, "count", "default");
        REQUIRE(value == "default");
    }
}

TEST_CASE("JSON getRequiredValue", "[json_utils]") {
    std::string json_str = R"({"name": "test", "count": 10})";
    auto json = parse(json_str);
    
    SECTION("Get existing value") {
        auto value = getRequiredValue<std::string>(json, "name");
        REQUIRE(value == "test");
    }
    
    SECTION("Get non-existing value throws exception") {
        REQUIRE_THROWS_AS(getRequiredValue<std::string>(json, "missing"),
                         JsonParseException);
    }
    
    SECTION("Get value with wrong type throws exception") {
        REQUIRE_THROWS_AS(getRequiredValue<std::string>(json, "count"),
                         JsonParseException);
    }
}

TEST_CASE("JSON toString", "[json_utils]") {
    nlohmann::json json = {{"key", "value"}, {"number", 42}};
    
    SECTION("Compact format") {
        auto str = toString(json, -1);
        REQUIRE_FALSE(str.empty());
        REQUIRE(str.find('\n') == std::string::npos);
    }
    
    SECTION("Pretty format") {
        auto str = toString(json, 2);
        REQUIRE_FALSE(str.empty());
        // Pretty format should contain newlines
        REQUIRE(str.find('\n') != std::string::npos);
    }
}

TEST_CASE("JSON hasRequiredKeys", "[json_utils]") {
    std::string json_str = R"({"name": "test", "count": 10, "active": true})";
    auto json = parse(json_str);
    
    SECTION("All required keys present") {
        REQUIRE(hasRequiredKeys(json, {"name", "count"}));
    }
    
    SECTION("Some required keys missing") {
        REQUIRE_FALSE(hasRequiredKeys(json, {"name", "missing"}));
    }
    
    SECTION("Empty required keys list") {
        REQUIRE(hasRequiredKeys(json, {}));
    }
    
    SECTION("Non-object JSON returns false") {
        nlohmann::json array_json = nlohmann::json::array();
        REQUIRE_FALSE(hasRequiredKeys(array_json, {"key"}));
    }
}

TEST_CASE("JSON createObject", "[json_utils]") {
    SECTION("Create object from pairs") {
        std::map<std::string, nlohmann::json> pairs = {
            {"name", "test"},
            {"count", 42},
            {"active", true}
        };
        
        auto json = createObject(pairs);
        
        REQUIRE(json.is_object());
        REQUIRE(json["name"] == "test");
        REQUIRE(json["count"] == 42);
        REQUIRE(json["active"] == true);
    }
    
    SECTION("Create empty object") {
        auto json = createObject({});
        REQUIRE(json.is_object());
        REQUIRE(json.empty());
    }
}
