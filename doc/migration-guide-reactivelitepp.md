# Migration Guide: Classic Getter/Setter → ReactiveLitepp Properties

This document records every decision made during the migration of the
`Light` class and serves as the authoritative reference for migrating
the remaining domain classes (`Sensor` and its subtypes, future `Room`,
`Scene`, etc.).

---

## 1. Why migrate?

| Before | After |
|--------|-------|
| Callers had to poll `getX()` methods | Callers subscribe once and are pushed updates |
| Errors were encoded in `Result<T>` return values | Errors propagate as typed exceptions |
| `std::optional<T>` used to express "not yet available" | Absence throws `ResourceNotFoundException` |
| Internal state was mutable via free public methods | All mutations go through observable `Property<T>` |

---

## 2. Architecture overview

```
┌──────────────────────────────────────────────────────┐
│                  Bridge / StateManager               │
│   SSEClient → updateFromEvent → OnResourceEvent      │
└─────────────────────┬────────────────────────────────┘
                      │  ScopedSubscription (in Light)
                      ▼
         ┌────────────────────────┐
         │         Light          │   : public ObservableObject
         │                        │
         │  ReadonlyProperty<T>   │  ← read-only from outside
         │  Property<T>           │  ← read/write from outside
         │                        │
         │  private impl methods  │  ← only called by properties
         └────────────────────────┘
                      │
                      ▼
             PropertyChanged event
          (subscribers notified on SSE
           or on explicit assignment)
```

---

## 3. Step-by-step migration checklist

### 3.1 Header (`include/hue4cpp/MyClass.h`)

#### a. Inheritance

```cpp
// Before
class MyClass {

// After
class MyClass : public ReactiveLitepp::ObservableObject {
```

`public` is mandatory. Private inheritance hides `PropertyChanged` /
`PropertyChanging` from subscribers, defeating the entire purpose.

#### b. Copy / move

Properties capture `this` in their lambdas. Copying or moving the
object would leave dangling pointers inside those lambdas.

```cpp
MyClass(const MyClass&) = delete;
MyClass& operator=(const MyClass&) = delete;
MyClass(MyClass&&)      = delete;
MyClass& operator=(MyClass&&) = delete;
```

#### c. New includes

```cpp
#include <ReactiveLitepp/ObservableObject.h>
#include <ReactiveLitepp/ScopedSubscription.h>
```

Forward-declare `ResourceEventArgs` to break the `state.h` ↔ class
circular include:

```cpp
struct ResourceEventArgs;
```

#### d. Backing fields

Rename every field that a property exposes from `foo` to `_foo`.

```cpp
// Before
std::string name;

// After
std::string _name;
```

#### e. Read-only properties

Use `ReadonlyProperty<T>` for values that are set only by the bridge
(identity, metadata).  The getter lambda reads from the backing field.
No setter lambda is provided.

```cpp
ReadonlyProperty<std::string> Name{
    [this]() { return _name; }
};
```

#### f. Read/write properties

Use `Property<T>` when callers are allowed to change the value (state,
brightness, color …).

- The **getter** calls the private impl method and re-throws
  `HueException` unchanged.
- The **setter** wraps `NotifyPropertyChanging`, the private impl call,
  and `NotifyPropertyChanged` in a try/catch that re-throws
  `HueException`.

```cpp
Property<uint8_t> Brightness{
    [this]() {
        try { return getBrightness(); }
        catch (const HueException&) { throw; }
    },
    [this](uint8_t& value) {
        try {
            NotifyPropertyChanging<&MyClass::Brightness>();
            setBrightness(value);
            NotifyPropertyChanged<&MyClass::Brightness>();
        }
        catch (const HueException&) { throw; }
    }
};
```

> **Why re-throw explicitly?**  
> A bare `catch (...)` swallowing the exception would leave the
> property in an indeterminate state and hide errors from the caller.
> Re-throwing preserves the typed exception across the lambda boundary.

#### g. Private impl methods

All methods that were previously public become `private`.  Only the
properties call them.

```cpp
private:
    uint8_t getBrightness() const;           // throws on failure
    void    setBrightness(uint8_t, TransitionTime);

    // SSE wiring
    void subscribeToBridgeEvents();
    void onResourceEvent(const ResourceEventArgs&);
    ReactiveLitepp::ScopedSubscription _bridgeEventSubscription;
```

Methods that used to return `Result<T>` now return `void` and throw:

```cpp
// Before
Result<void> turnOn();

// After  (private)
void turnOn(TransitionTime transition);   // throws HueException
```

---

### 3.2 Implementation (`src/MyClass.cpp`)

#### a. Constructors

Call `subscribeToBridgeEvents()` only in the constructor that receives a
`Bridge*`.  The default constructor leaves `_bridgeEventSubscription`
empty (safe to destroy).

```cpp
MyClass::MyClass(const std::string& id, Bridge* bridge)
    : _id(id), _bridge(bridge) {
    subscribeToBridgeEvents();
}
```

#### b. `subscribeToBridgeEvents()`

Use `SubscribeScoped` so the subscription is automatically cancelled
when the object is destroyed.  `ScopedSubscription` is move-assignable,
so storing the return value is sufficient.

```cpp
void MyClass::subscribeToBridgeEvents() {
    if (!_bridge) return;
    _bridgeEventSubscription =
        _bridge->getStateManager().OnResourceEvent.SubscribeScoped(
            [this](const ResourceEventArgs& e) { onResourceEvent(e); });
}
```

#### c. `onResourceEvent()`

Filter by both event type and resource id before touching any state.
Then use `SetPropertyValueAndNotify` for fields that store their value
internally (read-only properties, metadata), and plain
`NotifyPropertyChanged` for properties whose getter fetches live from
the bridge cache (state properties).

```cpp
void MyClass::onResourceEvent(const ResourceEventArgs& e) {
    // 1. Guard: only handle events for this object
    if (e.type != EventType::LightStateChanged || e.resource_id != _id)
        return;

    try {
        auto delta = nlohmann::json::parse(e.state_json);

        // 2a. Fields stored locally → update backing field AND notify
        if (delta.contains("metadata")) {
            auto name = delta["metadata"].value("name", "");
            SetPropertyValueAndNotify<&MyClass::Name>(_name, name);
        }

        // 2b. Fields fetched live → only notify (getter reads from cache)
        if (delta.contains("on"))
            NotifyPropertyChanged<&MyClass::IsOn>();

        if (delta.contains("dimming"))
            NotifyPropertyChanged<&MyClass::Brightness>();
    }
    catch (...) {
        // Malformed delta: notify everything so subscribers re-read
        NotifyPropertyChanged<&MyClass::IsOn>();
        NotifyPropertyChanged<&MyClass::Brightness>();
    }
}
```

**`SetPropertyValueAndNotify` vs `NotifyPropertyChanged`**

| Situation | Method to use |
|-----------|---------------|
| Backing field lives **inside** this object (e.g. `_name`) | `SetPropertyValueAndNotify<&Class::Prop>(field, newValue)` — performs equality check, assigns field, fires both Changing + Changed |
| Value lives in the **bridge cache** (e.g. on/off read via `getLightState`) | `NotifyPropertyChanged<&Class::Prop>()` — only fires Changed; getter will read fresh value from cache |

#### d. Exception contract for impl methods

Every private impl method returns `void` and throws `HueException`
subclasses:

| Condition | Exception |
|-----------|-----------|
| No `_bridge` pointer | `BridgeNotReachableException` |
| Bridge unauthenticated | `BridgeNotReachableException` / `AuthenticationException` |
| Value absent from cache after refresh | `ResourceNotFoundException` |
| Parameter out of range | `InvalidParameterException` |
| HTTP 401/403 | `AuthenticationException` |
| HTTP error | `NetworkException` |

#### e. `initFromJson()`

Use `SetPropertyValueAndNotify` so that callers who subscribed before
the first JSON population are notified of the initial values.

```cpp
void MyClass::initFromJson(const nlohmann::json& json) {
    auto idVal = json_utils::getValueOr<std::string>(json, "id", _id);
    SetPropertyValueAndNotify<&MyClass::Id>(_id, idVal);

    if (json.contains("metadata")) {
        auto name = json_utils::getValueOr<std::string>(
                        json["metadata"], "name", "");
        SetPropertyValueAndNotify<&MyClass::Name>(_name, name);
    }
    // ... capability detection ...

    // Always cache the full snapshot so property getters can read from it
    if (_bridge && !_id.empty())
        _bridge->getStateManager().setResourceState(_id, json.dump());
}
```

---

## 4. Two-tier notification strategy

```
Property type          | Source of truth   | SSE handler uses
-----------------------|-------------------|------------------------------------
ReadonlyProperty<T>    | _field in object  | SetPropertyValueAndNotify (+ equality guard)
Property<T> (metadata) | _field in object  | SetPropertyValueAndNotify
Property<T> (state)    | bridge JSON cache | NotifyPropertyChanged only
```

This keeps the object lean: it stores only what the bridge never pushes
as a delta (id, name, capabilities) and delegates live state to the
existing bridge cache.

---

## 5. Caller migration

### Reading a value

```cpp
// Before
auto b = light->getBrightness();      // returns std::optional<uint8_t>
if (b.has_value()) { use(*b); }

// After
try {
    uint8_t b = light->Brightness;    // implicit conversion operator
    use(b);
} catch (const ResourceNotFoundException& e) { /* not available */ }
  catch (const HueException& e)             { /* bridge error   */ }
```

### Writing a value

```cpp
// Before
auto result = light->setBrightness(80);
if (!result.isSuccess()) { log(result.error_message); }

// After
try {
    light->Brightness = 80;
} catch (const InvalidParameterException& e) { /* out of range  */ }
  catch (const HueException& e)              { /* bridge error  */ }
```

### Reacting to changes

```cpp
// Subscribe once; callback fires on every SSE-driven change
auto sub = light->PropertyChanged.SubscribeScoped(
    [](ReactiveLitepp::ObservableObject& obj,
       ReactiveLitepp::PropertyChangedArgs args) {
        if (args.PropertyName() == "Name")
            std::cout << "Renamed!\n";
        if (args.PropertyName() == "IsOn")
            std::cout << "Power toggled!\n";
    });
// sub goes out of scope → auto-unsubscribed
```

---

## 6. `Sensor` migration map

`Sensor` / `sensor_base.h` already has the structural skeleton
(`subscribeToBridgeEvents`, `onResourceEvent`,
`_bridgeEventSubscription`, `notifyStateProperties`).
The remaining work per sensor type is:

1. Add `ReadonlyProperty<std::string> Id` / `Name` (same as `Light`).
2. Add `ReadonlyProperty<T>` or `Property<T>` for each sensor reading
   (motion, temperature, light level …).
3. Implement `notifyStateProperties(delta)` to call
   `SetPropertyValueAndNotify` or `NotifyPropertyChanged` for the
   JSON keys specific to that sensor type.
4. Update `initFromJson` to call `SetPropertyValueAndNotify` for every
   field instead of assigning directly.
5. Remove all public getter/setter methods that are now covered by
   properties; keep only methods with no property equivalent
   (`getCapabilities`, `refresh`, `initFromJson`).
6. Add Catch2 tests following the three-section pattern from
   `light_tests.cpp`:
   - value updates and `PropertyChanged` fires,
   - absent key leaves value and no notification,
   - identical value leaves value and no notification.

---

## 7. Quick reference card

```cpp
// ── Header ───────────────────────────────────────────────────────────────
class Foo : public ReactiveLitepp::ObservableObject {
public:
    Foo(const std::string& id, Bridge*);
    ~Foo() = default;
    Foo(const Foo&) = delete; Foo& operator=(const Foo&) = delete;
    Foo(Foo&&)      = delete; Foo& operator=(Foo&&)      = delete;

    ReadonlyProperty<std::string> Id  { [this](){ return _id;   } };
    ReadonlyProperty<std::string> Name{ [this](){ return _name; } };

    Property<SomeType> Value{
        [this]() {
            try { return getValueImpl(); }
            catch (const HueException&) { throw; }
        },
        [this](SomeType& v) {
            try {
                NotifyPropertyChanging<&Foo::Value>();
                setValueImpl(v);
                NotifyPropertyChanged<&Foo::Value>();
            }
            catch (const HueException&) { throw; }
        }
    };

    void initFromJson(const nlohmann::json&);
    void refresh();

private:
    SomeType getValueImpl() const;           // throws HueException
    void     setValueImpl(SomeType);         // throws HueException

    void subscribeToBridgeEvents();
    void onResourceEvent(const ResourceEventArgs&);

    std::string  _id, _name;
    Bridge*      _bridge;
    ReactiveLitepp::ScopedSubscription _bridgeEventSubscription;
};

// ── .cpp ─────────────────────────────────────────────────────────────────
Foo::Foo(const std::string& id, Bridge* b) : _id(id), _bridge(b) {
    subscribeToBridgeEvents();
}
void Foo::subscribeToBridgeEvents() {
    if (!_bridge) return;
    _bridgeEventSubscription =
        _bridge->getStateManager().OnResourceEvent.SubscribeScoped(
            [this](const ResourceEventArgs& e){ onResourceEvent(e); });
}
void Foo::onResourceEvent(const ResourceEventArgs& e) {
    if (e.type != EventType::/*Relevant*/ || e.resource_id != _id) return;
    try {
        auto d = nlohmann::json::parse(e.state_json);
        if (d.contains("metadata"))
            SetPropertyValueAndNotify<&Foo::Name>(
                _name, d["metadata"].value("name", _name));
        if (d.contains("someKey"))
            NotifyPropertyChanged<&Foo::Value>();
    } catch (...) {
        NotifyPropertyChanged<&Foo::Name>();
        NotifyPropertyChanged<&Foo::Value>();
    }
}
void Foo::initFromJson(const nlohmann::json& j) {
    SetPropertyValueAndNotify<&Foo::Id>(_id, j.value("id", _id));
    if (j.contains("metadata"))
        SetPropertyValueAndNotify<&Foo::Name>(
            _name, j["metadata"].value("name", ""));
    if (_bridge && !_id.empty())
        _bridge->getStateManager().setResourceState(_id, j.dump());
}
```
