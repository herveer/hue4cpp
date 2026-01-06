#include "hue4cpp/json_utils.h"

namespace hue4cpp {
namespace json_utils {

nlohmann::json parse(const std::string& json_str) {
    try {
        return nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::parse_error& e) {
        throw JsonParseException("JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw JsonParseException("Unexpected error while parsing JSON: " + std::string(e.what()));
    }
}

std::string toString(const nlohmann::json& json, int indent) {
    try {
        return json.dump(indent);
    } catch (const std::exception& e) {
        throw JsonParseException("Failed to convert JSON to string: " + std::string(e.what()));
    }
}

bool hasRequiredKeys(const nlohmann::json& json,
                    const std::vector<std::string>& required_keys) {
    if (!json.is_object()) {
        return false;
    }
    
    for (const auto& key : required_keys) {
        if (!json.contains(key)) {
            return false;
        }
    }
    
    return true;
}

nlohmann::json createObject(const std::map<std::string, nlohmann::json>& pairs) {
    nlohmann::json result = nlohmann::json::object();
    for (const auto& [key, value] : pairs) {
        result[key] = value;
    }
    return result;
}

} // namespace json_utils
} // namespace hue4cpp
