#pragma once

#include "../types.h"
#include "../state.h"
#include "../exceptions.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <ReactiveLitepp/ObservableObject.h>
#include <ReactiveLitepp/ScopedSubscription.h>

/**
 * @file sensor_base.h
 * @brief Base class for all sensor types
 */

namespace hue4cpp {

// Forward declarations
class Bridge;
struct ResourceEventArgs;

/**
 * @brief Abstract base class for all Hue sensors
 *
 * Sensors are ObservableObjects. Subscribing to OnStateChanged gives
 * real-time property-change notifications whenever the bridge reports
 * a new reading via SSE.
 */
class Sensor : public ReactiveLitepp::ObservableObject {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~Sensor() = default;

    // Non-copyable, non-movable (ObservableObject subscriptions hold `this`)
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    Sensor(Sensor&&) = delete;
    Sensor& operator=(Sensor&&) = delete;

    /**
     * @brief Sensor unique identifier (read-only)
     */
    ReactiveLitepp::ReadonlyProperty<std::string> Id{
        [this]() { return _id; }
    };

    /**
     * @brief Get or set the sensor's display name
     *
     * Assigning a new value PUTs @c {"metadata":{"name":"\u2026"}} to the bridge
     * immediately. The bridge confirms the rename via an SSE event which
     * updates the backing field and fires @c PropertyChanged a second time.
     * @throws InvalidParameterException if the name is empty
     * @throws BridgeNotReachableException if the bridge is not available
     * @throws AuthenticationException if not authenticated
     */
    ReactiveLitepp::Property<std::string> Name{
        [this]() { return _name; },
        [this](std::string& value) {
            try {
                NotifyPropertyChanging<&Sensor::Name>();
                setName(value);
                NotifyPropertyChanged<&Sensor::Name>();
            }
            catch (const HueException&) { throw; }
        }
    };

    /**
     * @brief Compatibility helper – prefer the Id property for new code
     * @return Sensor ID
     */
    std::string getId() const { return _id; }

    /**
     * @brief Get the sensor's type
     * @return SensorType
     */
    virtual SensorType getType() const = 0;

    /**
     * @brief Check if sensor is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const;

    /**
     * @brief Refresh sensor state from bridge
     * @return Result indicating success or failure
     */
    Result<void> refresh();

    /**
     * @brief Update sensor state from JSON data
     * @param json JSON object containing sensor data from API
     * @note This is an internal method used by Bridge to populate sensor data
     */
    void initFromJson(const nlohmann::json& json);

    /**
     * @brief Fired whenever the bridge reports a state change for this sensor.
     *
     * Subscribe to be notified reactively instead of polling:
     * @code
     * auto sub = sensor->OnStateChanged.SubscribeScoped(
     *     [](const ResourceEventArgs& e) { ... });
     * @endcode
     */
    ReactiveLitepp::Event<const ResourceEventArgs&> OnStateChanged;

    /**
     * @brief Get the resource type string for this sensor (e.g., "motion", "temperature")
     * @return API resource type string
     */
    std::string getResourceTypeString() const;

protected:
    /**
     * @brief Protected constructor - only derived classes can construct
     * @param id Sensor unique identifier
     * @param bridge Pointer to parent bridge
     * @param type Sensor type
     */
    Sensor(const std::string& id, Bridge* bridge, SensorType type);

    /**
     * @brief Get the bridge pointer (for derived classes)
     * @return Pointer to parent bridge (may be nullptr)
     */
    Bridge* getBridge() const;

    std::string _id;
    std::string _name;
    Bridge* _bridge;
    SensorType _type;

private:
    void setName(const std::string& name);
    void sendUpdate(const nlohmann::json& state_update);
    /**
     * @brief Subscribe to StateManager::OnResourceEvent and set up notifications.
     * Called from the constructor when a bridge is available.
     */
    void subscribeToBridgeEvents();

    /**
     * @brief Handle a StateManager resource event.
     * Fires OnStateChanged and derived-class property notifications for this sensor.
     * @param e Unified resource event args
     */
    void onResourceEvent(const ResourceEventArgs& e);

    /**
     * @brief Called by onResourceEvent after the state has been confirmed to belong
     * to this sensor. Derived classes override this to fire their typed property
     * notifications based on the keys present in the delta JSON.
     * @param delta Parsed delta JSON object
     */
    virtual void notifyStateProperties(const nlohmann::json& delta) = 0;

    /// Scoped subscription to StateManager::OnResourceEvent.
    ReactiveLitepp::ScopedSubscription _bridgeEventSubscription;

    friend class Bridge;
};

/**
 * @brief Factory function to create appropriate sensor from JSON
 * @param json JSON object containing sensor data
 * @param bridge Pointer to parent bridge
 * @return Unique pointer to created sensor (nullptr if type unknown)
 */
std::unique_ptr<Sensor> createSensorFromJson(const nlohmann::json& json, Bridge* bridge);

} // namespace hue4cpp
