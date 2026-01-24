#include "hue4cpp/bridge.h"
#include "hue4cpp/light.h"
#include "hue4cpp/state.h"

namespace hue4cpp {

// Bridge::Impl definition
class Bridge::Impl {
public:
    BridgeInfo info;
    std::string auth_key;
    std::unique_ptr<StateManager> state_manager;
    
    Impl() : state_manager(std::make_unique<StateManager>()) {}
    explicit Impl(const BridgeInfo& bridge_info)
        : info(bridge_info), state_manager(std::make_unique<StateManager>()) {}
};

// Bridge implementation
Bridge::Bridge() : pImpl(std::make_unique<Impl>()) {}

Bridge::Bridge(const BridgeInfo& info) : pImpl(std::make_unique<Impl>(info)) {}

Bridge::~Bridge() = default;

Bridge::Bridge(Bridge&&) noexcept = default;
Bridge& Bridge::operator=(Bridge&&) noexcept = default;

// Discovery methods are implemented in discovery.cpp

Result<std::string> Bridge::authenticate(const std::string& app_name,
                                         const std::string& device_name) {
    // TODO: Implement authentication
    return Result<std::string>(ErrorCode::UnknownError, "Not implemented");
}

void Bridge::setAuthenticationKey(const std::string& key) {
    pImpl->auth_key = key;
}

bool Bridge::isAuthenticated() const {
    return !pImpl->auth_key.empty();
}

std::vector<Light> Bridge::getLights() {
    // TODO: Implement get lights
    return {};
}

std::optional<Light> Bridge::getLight(const std::string& light_id) {
    // TODO: Implement get light
    return std::nullopt;
}

const BridgeInfo& Bridge::getInfo() const {
    return pImpl->info;
}

StateManager& Bridge::getStateManager() {
    return *pImpl->state_manager;
}

// isReachable is implemented in discovery.cpp

} // namespace hue4cpp
