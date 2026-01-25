#include "hue4cpp/light.h"
#include "hue4cpp/bridge.h"
#include "hue4cpp/state.h"
#include "hue4cpp/http_client.h"
#include "hue4cpp/json_utils.h"
#include "hue4cpp/color_utils.h"
#include <cmath>

namespace hue4cpp {

namespace {
    // Color temperature limits for Hue lights (in mireds)
    constexpr uint16_t MIN_MIREDS = 153;  // ~6500K (cool white)
    constexpr uint16_t MAX_MIREDS = 500;  // ~2000K (warm white)
}
class Light::Impl {
public:
    std::string id;
    std::string name;
    Bridge* bridge;
    LightCapabilities capabilities;
    bool is_on;
    std::optional<uint8_t> brightness;
    std::optional<XYColor> color;
    std::optional<ColorTemperature> color_temp;
    
    Impl() : bridge(nullptr), is_on(false) {
        capabilities.on_off = true;
        capabilities.brightness = false;
        capabilities.color = false;
        capabilities.color_temperature = false;
        capabilities.effects = false;
    }
    
    Impl(const std::string& light_id, Bridge* parent_bridge)
        : id(light_id), bridge(parent_bridge), is_on(false) {
        capabilities.on_off = true;
        capabilities.brightness = false;
        capabilities.color = false;
        capabilities.color_temperature = false;
        capabilities.effects = false;
    }
    
    // Helper to send a PUT request to update light state
    Result<void> sendUpdate(const nlohmann::json& state_update) {
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
            
        } catch (const JsonParseException& e) {
            return Result<void>(ErrorCode::InvalidRequest, 
                std::string("JSON error: ") + e.what());
        } catch (const std::exception& e) {
            return Result<void>(ErrorCode::UnknownError, 
                std::string("Error: ") + e.what());
        }
    }
    
    // Helper to add transition time to update JSON
    void addTransitionTime(nlohmann::json& update, TransitionTime transition) {
        if (transition.count() > 0) {
            // Hue API V2 uses milliseconds for duration
            update["dynamics"] = {{"duration", static_cast<int>(transition.count())}};
        }
    }
};

// Light implementation
Light::Light() : pImpl(std::make_unique<Impl>()) {}

Light::Light(const std::string& id, Bridge* bridge)
    : pImpl(std::make_unique<Impl>(id, bridge)) {}

Light::~Light() = default;

Light::Light(const Light& other) : pImpl(std::make_unique<Impl>(*other.pImpl)) {}

Light& Light::operator=(const Light& other) {
    if (this != &other) {
        pImpl = std::make_unique<Impl>(*other.pImpl);
    }
    return *this;
}

Light::Light(Light&&) noexcept = default;
Light& Light::operator=(Light&&) noexcept = default;

std::string Light::getId() const {
    return pImpl->id;
}

std::string Light::getName() const {
    return pImpl->name;
}

LightCapabilities Light::getCapabilities() const {
    return pImpl->capabilities;
}

bool Light::isOn() const {
    // Try to get state from StateManager cache first
    if (pImpl->bridge) {
        auto& state_manager = pImpl->bridge->getStateManager();
        auto cached_state = state_manager.getLightState(pImpl->id);
        
        if (!cached_state.empty()) {
            try {
                auto state_json = json_utils::parse(cached_state);
                if (state_json.contains("on") && state_json["on"].is_object()) {
                    auto on_obj = state_json["on"];
                    if (on_obj.contains("on") && on_obj["on"].is_boolean()) {
                        return on_obj["on"].template get<bool>();
                    }
                }
            } catch (...) {
                // Fall through to return cached value
            }
        }
    }
    
    // Fall back to locally cached value
    return pImpl->is_on;
}

Result<void> Light::turnOn(TransitionTime transition) {
    if (!pImpl->capabilities.on_off) {
        return Result<void>(ErrorCode::InvalidRequest, 
            "Light does not support on/off control");
    }
    
    nlohmann::json update;
    update["on"] = {{"on", true}};
    pImpl->addTransitionTime(update, transition);
    
    auto result = pImpl->sendUpdate(update);
    if (result.isSuccess()) {
        pImpl->is_on = true;
    }
    return result;
}

Result<void> Light::turnOff(TransitionTime transition) {
    if (!pImpl->capabilities.on_off) {
        return Result<void>(ErrorCode::InvalidRequest, 
            "Light does not support on/off control");
    }
    
    nlohmann::json update;
    update["on"] = {{"on", false}};
    pImpl->addTransitionTime(update, transition);
    
    auto result = pImpl->sendUpdate(update);
    if (result.isSuccess()) {
        pImpl->is_on = false;
    }
    return result;
}

Result<void> Light::toggle(TransitionTime transition) {
    if (pImpl->is_on) {
        return turnOff(transition);
    } else {
        return turnOn(transition);
    }
}

std::optional<uint8_t> Light::getBrightness() const {
    // Try to get state from StateManager cache first
    if (pImpl->bridge) {
        auto& state_manager = pImpl->bridge->getStateManager();
        auto cached_state = state_manager.getLightState(pImpl->id);
        
        if (!cached_state.empty()) {
            try {
                auto state_json = json_utils::parse(cached_state);
                if (state_json.contains("dimming") && state_json["dimming"].is_object()) {
                    auto dimming = state_json["dimming"];
                    if (dimming.contains("brightness") && dimming["brightness"].is_number()) {
                        double brightness_val = dimming["brightness"].template get<double>();
                        return static_cast<uint8_t>(std::round(brightness_val));
                    }
                }
            } catch (...) {
                // Fall through to return cached value
            }
        }
    }
    
    // Fall back to locally cached value
    return pImpl->brightness;
}

Result<void> Light::setBrightness(uint8_t brightness, TransitionTime transition) {
    if (!pImpl->capabilities.brightness) {
        return Result<void>(ErrorCode::InvalidRequest, 
            "Light does not support brightness control");
    }
    
    if (brightness > 100) {
        return Result<void>(ErrorCode::InvalidParameter, 
            "Brightness must be between 0 and 100");
    }
    
    nlohmann::json update;
    update["dimming"] = {{"brightness", static_cast<double>(brightness)}};
    pImpl->addTransitionTime(update, transition);
    
    auto result = pImpl->sendUpdate(update);
    if (result.isSuccess()) {
        pImpl->brightness = brightness;
    }
    return result;
}

std::optional<XYColor> Light::getColor() const {
    // Try to get state from StateManager cache first
    if (pImpl->bridge) {
        auto& state_manager = pImpl->bridge->getStateManager();
        auto cached_state = state_manager.getLightState(pImpl->id);
        
        if (!cached_state.empty()) {
            try {
                auto state_json = json_utils::parse(cached_state);
                if (state_json.contains("color") && state_json["color"].is_object()) {
                    auto color_obj = state_json["color"];
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
            } catch (...) {
                // Fall through to return cached value
            }
        }
    }
    
    // Fall back to locally cached value
    return pImpl->color;
}

Result<void> Light::setColor(const XYColor& color, TransitionTime transition) {
    if (!pImpl->capabilities.color) {
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
    pImpl->addTransitionTime(update, transition);
    
    auto result = pImpl->sendUpdate(update);
    if (result.isSuccess()) {
        pImpl->color = color;
    }
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
    // Try to get state from StateManager cache first
    if (pImpl->bridge) {
        auto& state_manager = pImpl->bridge->getStateManager();
        auto cached_state = state_manager.getLightState(pImpl->id);
        
        if (!cached_state.empty()) {
            try {
                auto state_json = json_utils::parse(cached_state);
                if (state_json.contains("color_temperature") && 
                    state_json["color_temperature"].is_object()) {
                    auto ct_obj = state_json["color_temperature"];
                    if (ct_obj.contains("mirek") && ct_obj["mirek"].is_number()) {
                        uint16_t mirek = static_cast<uint16_t>(ct_obj["mirek"].template get<int>());
                        return ColorTemperature(mirek);
                    }
                }
            } catch (...) {
                // Fall through to return cached value
            }
        }
    }
    
    // Fall back to locally cached value
    return pImpl->color_temp;
}

Result<void> Light::setColorTemperature(const ColorTemperature& temperature,
                                        TransitionTime transition) {
    if (!pImpl->capabilities.color_temperature) {
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
    pImpl->addTransitionTime(update, transition);
    
    auto result = pImpl->sendUpdate(update);
    if (result.isSuccess()) {
        pImpl->color_temp = temperature;
    }
    return result;
}

Result<void> Light::alert() {
    nlohmann::json update;
    update["alert"] = {
        {"action", "breathe"}
    };
    
    return pImpl->sendUpdate(update);
}

Result<void> Light::refresh() {
    if (!pImpl->bridge || !pImpl->bridge->isAuthenticated()) {
        return Result<void>(ErrorCode::AuthenticationRequired, 
            "Bridge not authenticated");
    }
    
    auto light_opt = pImpl->bridge->getLight(pImpl->id);
    if (!light_opt.has_value()) {
        return Result<void>(ErrorCode::ResourceNotFound, 
            "Failed to refresh light state");
    }
    
    // Copy the refreshed state
    *pImpl = *light_opt->pImpl;
    return Result<void>();
}

void Light::updateFromJson(const nlohmann::json& json) {
    // Extract basic properties
    pImpl->id = json_utils::getValueOr<std::string>(json, "id", pImpl->id);
    
    // Extract metadata
    if (json.contains("metadata") && json["metadata"].is_object()) {
        auto metadata = json["metadata"];
        pImpl->name = json_utils::getValueOr<std::string>(metadata, "name", "");
    }
    
    // Extract on/off state
    if (json.contains("on") && json["on"].is_object()) {
        auto on = json["on"];
        pImpl->is_on = json_utils::getValueOr<bool>(on, "on", false);
    }
    
    // Extract brightness (dimming)
    if (json.contains("dimming") && json["dimming"].is_object()) {
        auto dimming = json["dimming"];
        auto brightness_val = json_utils::getValue<double>(dimming, "brightness");
        if (brightness_val.has_value()) {
            pImpl->brightness = static_cast<uint8_t>(brightness_val.value());
            pImpl->capabilities.brightness = true;
        }
    }
    
    // Extract color (XY)
    if (json.contains("color") && json["color"].is_object()) {
        auto color = json["color"];
        if (color.contains("xy") && color["xy"].is_object()) {
            auto xy = color["xy"];
            auto x = json_utils::getValue<double>(xy, "x");
            auto y = json_utils::getValue<double>(xy, "y");
            if (x.has_value() && y.has_value()) {
                pImpl->color = XYColor(static_cast<float>(x.value()), 
                                      static_cast<float>(y.value()));
                pImpl->capabilities.color = true;
            }
        }
    }
    
    // Extract color temperature
    if (json.contains("color_temperature") && json["color_temperature"].is_object()) {
        auto color_temp = json["color_temperature"];
        auto mirek = json_utils::getValue<int>(color_temp, "mirek");
        if (mirek.has_value()) {
            pImpl->color_temp = ColorTemperature(static_cast<uint16_t>(mirek.value()));
            pImpl->capabilities.color_temperature = true;
        }
    }
    
    // Determine capabilities from what's present in the light data
    // On/off is always available
    pImpl->capabilities.on_off = true;
    
    // Effects capability (check if effects key exists)
    if (json.contains("effects")) {
        pImpl->capabilities.effects = true;
    }
}

} // namespace hue4cpp
