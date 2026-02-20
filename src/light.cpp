#include "hue4cpp/light.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/color_utils.h"
#include "hue4cpp/state.h"
#include <cmath>
#include <iostream>

namespace hue4cpp {

namespace {
    // Color temperature limits for Hue lights (in mireds)
    constexpr uint16_t MIN_MIREDS = 153;  // ~6500K (cool white)
    constexpr uint16_t MAX_MIREDS = 500;  // ~2000K (warm white)
}

// Light implementation
Light::Light() : bridge(nullptr) {
    capabilities.on_off = true;
    capabilities.brightness = false;
    capabilities.color = false;
    capabilities.color_temperature = false;
    capabilities.effects = false;
}

Light::Light(const std::string& light_id, Bridge* parent_bridge)
    : id(light_id), bridge(parent_bridge) {
    capabilities.on_off = true;
    capabilities.brightness = false;
    capabilities.color = false;
    capabilities.color_temperature = false;
    capabilities.effects = false;
}

std::string Light::getId() const {
    return id;
}

std::string Light::getName() const {
    return name;
}

LightCapabilities Light::getCapabilities() const {
    return capabilities;
}

bool Light::isOn() const {
    if (!bridge) {
        // No bridge and no cached state - return false
        return false;
    }

    std::function extractIsOn = [this](std::string state_json) -> std::optional<bool> {
        if (!state_json.empty()) {
            try {
                auto state = json_utils::parse(state_json);
                if (state.contains("on") && state["on"].is_object()) {
                    auto on_obj = state["on"];
                    if (on_obj.contains("on") && on_obj["on"].is_boolean()) {
                        return on_obj["on"].template get<bool>();
                    }
                }
            }
            catch (...) {
                // Fall through to return nullopt
            }
        }
        return std::nullopt;
    };

    try {
        // Ask bridge for light state (cache-first, API-fallback)
        std::string state_json = bridge->getLightState(id, false);
        auto is_on_opt = extractIsOn(state_json);
        if (is_on_opt.has_value()) {
            return is_on_opt.value();
        }

        // Try refreshing cache
        state_json = bridge->getLightState(id, true);
        is_on_opt = extractIsOn(state_json);
        return is_on_opt.value_or(false);
    }
    catch (...) {
        // If bridge access fails, return false
        return false;
    }
}

Result<void> Light::turnOn(TransitionTime transition) {
    if (!capabilities.on_off) {
        return Result<void>(ErrorCode::InvalidRequest,
            "Light does not support on/off control");
    }

    nlohmann::json update;
    update["on"] = { {"on", true} };
    addTransitionTime(update, transition);

    auto result = sendUpdate(update);
    return result;
}

Result<void> Light::turnOff(TransitionTime transition) {
    if (!capabilities.on_off) {
        return Result<void>(ErrorCode::InvalidRequest,
            "Light does not support on/off control");
    }

    nlohmann::json update;
    update["on"] = { {"on", false} };
    addTransitionTime(update, transition);

    auto result = sendUpdate(update);
    return result;
}

Result<void> Light::toggle(TransitionTime transition) {
    if (isOn()) {
        return turnOff(transition);
    }
    else {
        return turnOn(transition);
    }
}

std::optional<uint8_t> Light::getBrightness() const {
    std::function extractBrightness = [this](std::string state_json) -> std::optional<uint8_t> {
        if (!state_json.empty()) {
            try {
                auto state = json_utils::parse(state_json);
                if (state.contains("dimming") && state["dimming"].is_object()) {
                    auto dimming = state["dimming"];
                    if (dimming.contains("brightness") && dimming["brightness"].is_number()) {
                        double brightness_val = dimming["brightness"].template get<double>();
                        return static_cast<uint8_t>(brightness_val);
                    }
                }
            }
            catch (...) {
                // Fall through to refresh cache or return nullopt
            }
        }
        return std::nullopt;
    };

    if (!bridge) {
        // No bridge - cannot get state
        return std::nullopt;
    }

    // Ask bridge for light state (cache-first, API-fallback)
    std::string state_json = bridge->getLightState(id, false);
    auto brightness_opt = extractBrightness(state_json);
    if (brightness_opt.has_value()) {
        return brightness_opt;
    }

    // Try refreshing cache
    state_json = bridge->getLightState(id, true);
    brightness_opt = extractBrightness(state_json);
    return brightness_opt;
}

Result<void> Light::setBrightness(uint8_t brightness, TransitionTime transition) {
    if (!capabilities.brightness) {
        return Result<void>(ErrorCode::InvalidRequest,
            "Light does not support brightness control");
    }

    if (brightness > 100) {
        return Result<void>(ErrorCode::InvalidParameter,
            "Brightness must be between 0 and 100");
    }

    nlohmann::json update;
    update["dimming"] = { {"brightness", static_cast<double>(brightness)} };
    addTransitionTime(update, transition);

    auto result = sendUpdate(update);
    return result;
}

std::optional<XYColor> Light::getColor() const {
    std::function extractColor = [this](std::string state_json) -> std::optional<XYColor> {
        if (!state_json.empty()) {
            try {
                auto state = json_utils::parse(state_json);
                if (state.contains("color") && state["color"].is_object()) {
                    auto color_obj = state["color"];
                    if (color_obj.contains("xy") && color_obj["xy"].is_object()) {
                        auto xy = color_obj["xy"];
                        if (xy.contains("x") && xy.contains("y") &&
                            xy["x"].is_number() && xy["y"].is_number()) {
                            XYColor color;
                            color.x = static_cast<float>(xy["x"].template get<double>());
                            color.y = static_cast<float>(xy["y"].template get<double>());
                            return color;
                        }
                    }
                }
            }
            catch (...) {
                // Fall through to refresh cache or return nullopt
            }
        }
        return std::nullopt;
    };

    // Ask bridge for light state (cache-first, API-fallback)
    if (!bridge) {
        // No bridge - cannot get state
        return std::nullopt;
    }

    std::string state_json = bridge->getLightState(id, false);
    auto color_opt = extractColor(state_json);
    if (color_opt.has_value()) {
        return color_opt;
    }

    // Try refreshing cache
    state_json = bridge->getLightState(id, true);
    color_opt = extractColor(state_json);
    return color_opt;
}

Result<void> Light::setColor(const XYColor& color, TransitionTime transition) {
    if (!capabilities.color) {
        return Result<void>(ErrorCode::InvalidRequest,
            "Light does not support color control");
    }

    // Validate XY values (should be between 0 and 1)
    if (color.x < 0.0f || color.x > 1.0f || color.y < 0.0f || color.y > 1.0f) {
        return Result<void>(ErrorCode::InvalidParameter,
            "XY color values must be between 0.0 and 1.0");
    }

    nlohmann::json update;
    update["color"] = {
        {"xy", {
            {"x", static_cast<double>(color.x)},
            {"y", static_cast<double>(color.y)}
        }}
    };
    addTransitionTime(update, transition);

    auto result = sendUpdate(update);
    return result;
}

Result<void> Light::setColor(const RGBColor& color, TransitionTime transition) {
    // Convert RGB to XY using the color_utils function
    XYColor xy = color_utils::rgbToXy(color);
    return setColor(xy, transition);
}

Result<void> Light::setColor(float r, float g, float b, TransitionTime transition) {
    return setColor(RGBColor(r, g, b), transition);
}

std::optional<ColorTemperature> Light::getColorTemperature() const {    
    std::function extractColorTemperature = [this](std::string state_json) -> std::optional<ColorTemperature> {
        if (!state_json.empty()) {
            try {
                auto state = json_utils::parse(state_json);
                if (state.contains("color_temperature") &&
                    state["color_temperature"].is_object()) {
                    auto ct_obj = state["color_temperature"];
                    if (ct_obj.contains("mirek") && ct_obj["mirek"].is_number()) {
                        uint16_t mirek = static_cast<uint16_t>(ct_obj["mirek"].template get<int>());
                        return ColorTemperature(mirek);
                    }
                }
            }
            catch (...) {
                // Fall through to refresh cache or return nullopt
            }
        }
        return std::nullopt;
    };

    if(!bridge) {
        // No bridge - cannot get state
        return std::nullopt;
    }

    std::string state_json = bridge->getLightState(id, false);
    auto colorTemperature_opt = extractColorTemperature(state_json);
    if (colorTemperature_opt.has_value()) {
        return colorTemperature_opt;
    }

    // Try refreshing cache
    state_json = bridge->getLightState(id, true);
    colorTemperature_opt = extractColorTemperature(state_json);
    return colorTemperature_opt;
}

Result<void> Light::setColorTemperature(const ColorTemperature& temperature,
    TransitionTime transition) {
    if (!capabilities.color_temperature) {
        return Result<void>(ErrorCode::InvalidRequest,
            "Light does not support color temperature control");
    }

    // Validate mireds value using constants
    if (temperature.mireds < MIN_MIREDS || temperature.mireds > MAX_MIREDS) {
        return Result<void>(ErrorCode::InvalidParameter,
            "Color temperature must be between " + std::to_string(MIN_MIREDS) +
            " and " + std::to_string(MAX_MIREDS) + " mireds");
    }

    nlohmann::json update;
    update["color_temperature"] = {
        {"mirek", static_cast<int>(temperature.mireds)}
    };
    addTransitionTime(update, transition);

    auto result = sendUpdate(update);
    return result;
}

Result<void> Light::alert() {
    nlohmann::json update;
    update["alert"] = {
        {"action", "breathe"}
    };

    return sendUpdate(update);
}

Result<void> Light::refresh() {
    if (!bridge || !bridge->isAuthenticated()) {
        return Result<void>(ErrorCode::AuthenticationRequired,
            "Bridge not authenticated");
    }

    auto light_opt = bridge->getLight(id);
    if (!light_opt.has_value()) {
        return Result<void>(ErrorCode::ResourceNotFound,
            "Failed to refresh light state");
    }

    // Copy the refreshed state (but keep our bridge pointer)
    const auto& refreshed = light_opt.value();
    name = refreshed.name;
    capabilities = refreshed.capabilities;
    // Note: id and bridge remain unchanged
    return Result<void>();
}

void Light::initFromJson(const nlohmann::json& json) {
    // Extract basic properties
    id = json_utils::getValueOr<std::string>(json, "id", id);

    // Extract metadata
    if (json.contains("metadata") && json["metadata"].is_object()) {
        auto metadata = json["metadata"];
        name = json_utils::getValueOr<std::string>(metadata, "name", "");
    }

    // Extract and cache on/off state
    if (json.contains("on") && json["on"].is_object()) {
        auto on_obj = json["on"];
        if (on_obj.contains("on") && on_obj["on"].is_boolean()) {
            capabilities.on_off = true;
        }
    }

    // Extract and cache brightness (dimming)
    if (json.contains("dimming") && json["dimming"].is_object()) {
        auto dimming = json["dimming"];
        auto brightness_val = json_utils::getValue<double>(dimming, "brightness");
        if (brightness_val.has_value()) {
            capabilities.brightness = true;
        }
    }

    // Extract and cache color (XY)
    if (json.contains("color") && json["color"].is_object()) {
        auto color = json["color"];
        if (color.contains("xy") && color["xy"].is_object()) {
            auto xy = color["xy"];
            auto x = json_utils::getValue<double>(xy, "x");
            auto y = json_utils::getValue<double>(xy, "y");
            if (x.has_value() && y.has_value()) {
                capabilities.color = true;
            }
        }
    }

    // Extract and cache color temperature
    if (json.contains("color_temperature") && json["color_temperature"].is_object()) {
        auto color_temp = json["color_temperature"];
        auto mirek = json_utils::getValue<int>(color_temp, "mirek");
        if (mirek.has_value()) {
            capabilities.color_temperature = true;
        }
    }

    // Determine capabilities from what's present in the light data
    // On/off is always available
    capabilities.on_off = true;

    // Effects capability (check if effects key exists)
    if (json.contains("effects")) {
        capabilities.effects = true;
    }

    // Cache the full JSON state in StateManager if we have a bridge
    // This allows the getters to retrieve the state from cache
    if (bridge && !id.empty()) {
        bridge->getStateManager().setResourceState(id, json.dump());
    }
}

// Helper to send a PUT request to update light state
Result<void> Light::sendUpdate(const nlohmann::json& state_update) {
    if (!bridge || !bridge->isAuthenticated()) {
        return Result<void>(ErrorCode::AuthenticationRequired,
            "Bridge not authenticated");
    }

    const auto& bridge_info = bridge->getInfo();
    if (bridge_info.ip_address.empty() || id.empty()) {
        return Result<void>(ErrorCode::InvalidParameter,
            "Bridge IP or light ID not set");
    }

    try {
        HttpClient client;
        client.setVerifySsl(false);
        client.setTimeout(std::chrono::milliseconds(5000));

        std::string url = "https://" + bridge_info.ip_address +
            "/clip/v2/resource/light/" + id;

        std::map<std::string, std::string> headers;
        headers["hue-application-key"] = bridge->getAuthenticationKey();

        std::string body = json_utils::toString(state_update);
        auto response = client.put(url, body, headers);

        if (!response.isSuccess()) {
            if (response.status_code == 401 || response.status_code == 403) {
                return Result<void>(ErrorCode::AuthenticationFailed,
                    "Authentication failed");
            }
            return Result<void>(ErrorCode::NetworkError,
                "HTTP request failed: " + response.error_message);
        }

        // Parse response to check for errors
        auto json_response = json_utils::parse(response.body);

        if (json_response.contains("errors") && json_response["errors"].is_array()) {
            auto errors = json_response["errors"];
            if (!errors.empty()) {
                std::string error_desc = json_utils::getValueOr<std::string>(
                    errors[0], "description", "Unknown error");
                return Result<void>(ErrorCode::InvalidRequest, error_desc);
            }
        }

        return Result<void>();

    }
    catch (const JsonParseException& e) {
        return Result<void>(ErrorCode::InvalidRequest,
            std::string("JSON error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Result<void>(ErrorCode::UnknownError,
            std::string("Error: ") + e.what());
    }
}

// Helper to add transition time to update JSON
void Light::addTransitionTime(nlohmann::json& update, TransitionTime transition) {
    if (transition.count() > 0) {
        // Hue API V2 uses milliseconds for duration
        update["dynamics"] = { {"duration", static_cast<int>(transition.count())} };
    }
}

} // namespace hue4cpp
