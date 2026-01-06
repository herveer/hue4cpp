#include "hue4cpp/state.h"
#include "hue4cpp/bridge.h"
#include <map>
#include <atomic>

namespace hue4cpp {

// StateManager::Impl definition
class StateManager::Impl {
public:
    Bridge* bridge;
    std::atomic<bool> running;
    std::map<uint64_t, EventCallback> callbacks;
    uint64_t next_callback_id;
    std::map<std::string, std::string> light_states; // light_id -> JSON state
    
    Impl() : bridge(nullptr), running(false), next_callback_id(1) {}
    explicit Impl(Bridge* parent_bridge)
        : bridge(parent_bridge), running(false), next_callback_id(1) {}
};

// StateManager implementation
StateManager::StateManager() : pImpl(std::make_unique<Impl>()) {}

StateManager::StateManager(Bridge* bridge) : pImpl(std::make_unique<Impl>(bridge)) {}

StateManager::~StateManager() {
    stop();
}

StateManager::StateManager(StateManager&&) noexcept = default;
StateManager& StateManager::operator=(StateManager&&) noexcept = default;

Result<void> StateManager::start() {
    // TODO: Implement SSE connection
    pImpl->running = true;
    return Result<void>(ErrorCode::UnknownError, "Not implemented");
}

void StateManager::stop() {
    // TODO: Implement SSE disconnection
    pImpl->running = false;
}

bool StateManager::isRunning() const {
    return pImpl->running;
}

uint64_t StateManager::registerCallback(EventCallback callback) {
    uint64_t id = pImpl->next_callback_id++;
    pImpl->callbacks[id] = callback;
    return id;
}

void StateManager::unregisterCallback(uint64_t callback_id) {
    pImpl->callbacks.erase(callback_id);
}

std::string StateManager::getLightState(const std::string& light_id) const {
    auto it = pImpl->light_states.find(light_id);
    if (it != pImpl->light_states.end()) {
        return it->second;
    }
    return "";
}

void StateManager::updateFromEvent(const std::string& event_json) {
    // TODO: Parse event JSON and update internal state
    // TODO: Notify callbacks
}

} // namespace hue4cpp
