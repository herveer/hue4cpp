#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <stdexcept>

/**
 * @file json_utils.h
 * @brief JSON parsing and serialization utilities
 */

namespace hue4cpp {

/**
 * @brief Exception thrown when JSON parsing fails
 */
class JsonParseException : public std::runtime_error {
public:
    explicit JsonParseException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Utility functions for JSON handling
 */
namespace json_utils {

    /**
     * @brief Parse a JSON string
     * @param json_str The JSON string to parse
     * @return Parsed JSON object
     * @throws JsonParseException if parsing fails
     */
    nlohmann::json parse(const std::string& json_str);
    
    /**
     * @brief Safely get a value from a JSON object
     * @tparam T The type to extract
     * @param json The JSON object
     * @param key The key to look up
     * @return Optional value if key exists and has correct type
     */
    template<typename T>
    std::optional<T> getValue(const nlohmann::json& json, const std::string& key) {
        if (!json.is_object() || !json.contains(key)) {
            return std::nullopt;
        }
        
        try {
            return json[key].get<T>();
        } catch (const nlohmann::json::type_error&) {
            // Type mismatch
            return std::nullopt;
        } catch (const nlohmann::json::exception&) {
            // Other JSON errors
            return std::nullopt;
        }
    }
    
    /**
     * @brief Get a value from a JSON object with a default
     * @tparam T The type to extract
     * @param json The JSON object
     * @param key The key to look up
     * @param default_value The default value if key doesn't exist
     * @return The value or default
     */
    template<typename T>
    T getValueOr(const nlohmann::json& json, const std::string& key, const T& default_value) {
        auto value = getValue<T>(json, key);
        return value.value_or(default_value);
    }
    
    /**
     * @brief Get a required value from a JSON object
     * @tparam T The type to extract
     * @param json The JSON object
     * @param key The key to look up
     * @return The value
     * @throws JsonParseException if key doesn't exist or has wrong type
     */
    template<typename T>
    T getRequiredValue(const nlohmann::json& json, const std::string& key) {
        if (!json.is_object() || !json.contains(key)) {
            throw JsonParseException("Required key '" + key + "' not found in JSON object");
        }
        
        try {
            return json[key].get<T>();
        } catch (const std::exception& e) {
            throw JsonParseException("Failed to extract value for key '" + key + "': " + e.what());
        }
    }
    
    /**
     * @brief Convert a JSON object to a formatted string
     * @param json The JSON object
     * @param indent Number of spaces for indentation (-1 for compact)
     * @return Formatted JSON string
     */
    std::string toString(const nlohmann::json& json, int indent = -1);
    
    /**
     * @brief Check if a JSON object has all required keys
     * @param json The JSON object
     * @param required_keys List of required keys
     * @return true if all keys exist, false otherwise
     */
    bool hasRequiredKeys(const nlohmann::json& json,
                        const std::vector<std::string>& required_keys);
    
    /**
     * @brief Create a JSON object from key-value pairs
     * @param pairs Map of key-value pairs
     * @return JSON object
     */
    nlohmann::json createObject(const std::map<std::string, nlohmann::json>& pairs);

} // namespace json_utils

} // namespace hue4cpp
