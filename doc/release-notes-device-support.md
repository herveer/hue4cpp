# Release Notes — Device Support (v1.1.0)

> **Summary** — hue4cpp gains a first-class `Device` resource. A single
> `hue4cpp::Device` class represents every physical Hue product (bulb, plug,
> dimmer switch, motion sensor, …), aggregating the `Light` and `Sensor`
> objects it owns together with its product metadata (model, archetype,
> manufacturer, software version, certification). Two new bridge entry points,
> `Bridge::getDevices()` and `Bridge::getDevice(id)`, expose them. This is a
> purely **additive, non-breaking** change: no existing API was modified.

**Date:** 2026-06-08 · **Type:** Feature · **Compatibility:** Backward compatible

---

## 1. Overview

The Hue V2 API models a physical product as a *device* under
`/clip/v2/resource/device`. A device does not itself carry light or sensor
state; instead it references the *services* (light, motion, temperature,
button, …) that provide control and state, plus a block of product metadata.

Until now hue4cpp exposed those services directly (`Bridge::getLights()`,
`Bridge::getSensors()`) with no way to know which ones belonged to the same
physical unit. The new `Device` class closes that gap: it groups the owned
`Light` and `Sensor` objects and surfaces the product metadata in one place.

The lights and sensors held by a device are the **same reactive objects** that
`getLights()` / `getSensors()` return — subscribing to their properties still
delivers real-time SSE updates.

---

## 2. What's new (at a glance)

| Addition | Kind | Header |
|----------|------|--------|
| `hue4cpp::Device` | class | `include/hue4cpp/device.h` |
| `Bridge::getDevices()` | method | `include/hue4cpp/bridge.h` |
| `Bridge::getDevice(const std::string& id)` | method | `include/hue4cpp/bridge.h` |
| `device_listing` | example | `examples/device_listing.cpp` |

`device.h` is included by the umbrella header, so `#include <hue4cpp/hue4cpp.h>`
is enough to use the new API.

---

## 3. The `Device` class

### 3.1 Synopsis

```cpp
namespace hue4cpp {

class Device : public ReactiveLitepp::ObservableObject {
public:
    Device();
    Device(const std::string& id, Bridge* bridge);
    ~Device();

    // Non-copyable, non-movable (an ObservableObject that owns non-movable
    // Light/Sensor objects). Always handled through std::unique_ptr.
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    // Product metadata (read-only, reactive)
    ReadonlyProperty<std::string> Id;
    ReadonlyProperty<std::string> Type;             // always "device"
    ReadonlyProperty<std::string> Name;             // user-assigned name
    ReadonlyProperty<std::string> ModelId;
    ReadonlyProperty<std::string> ManufacturerName;
    ReadonlyProperty<std::string> ProductName;
    ReadonlyProperty<std::string> ProductArchetype;
    ReadonlyProperty<bool>        Certified;
    ReadonlyProperty<std::string> SoftwareVersion;

    // Owned services
    const std::vector<std::unique_ptr<Light>>&  getLights()  const;
    const std::vector<std::unique_ptr<Sensor>>& getSensors() const;

    // Internal: populate metadata from an API DeviceGet object
    void initFromJson(const nlohmann::json& json);
};

} // namespace hue4cpp
```

### 3.2 Properties

All metadata is exposed as `ReactiveLitepp::ReadonlyProperty`, identical in
spirit to the read-only properties on `Light` and `Sensor`. Read with the
implicit conversion or `.Get()`:

```cpp
std::string name = device->Name;          // implicit conversion
std::string sw   = device->SoftwareVersion.Get();
bool certified   = device->Certified;
```

| Property | Type | Source field | Description |
|----------|------|--------------|-------------|
| `Id` | `std::string` | `id` | Resource UUID (`^[0-9a-f]{8}-…$`) |
| `Type` | `std::string` | `type` | Resource type — always `"device"` |
| `Name` | `std::string` | `metadata.name` | Human-readable, user-editable name |
| `ModelId` | `std::string` | `product_data.model_id` | Device model identifier (e.g. `LCB001`) |
| `ManufacturerName` | `std::string` | `product_data.manufacturer_name` | Manufacturer (e.g. `Signify Netherlands B.V.`) |
| `ProductName` | `std::string` | `product_data.product_name` | Product name (e.g. `Hue color downlight`) |
| `ProductArchetype` | `std::string` | `product_data.product_archetype` | Archetype enum value as a string (e.g. `flood_bulb`) |
| `Certified` | `bool` | `product_data.certified` | Whether the device is Hue certified |
| `SoftwareVersion` | `std::string` | `product_data.software_version` | Firmware version (e.g. `1.122.8`) |

Because `Device` derives from `ObservableObject`, every property participates
in `PropertyChanged` notifications. `initFromJson()` only fires a change when a
value actually differs (equality-guarded via `SetPropertyValueAndNotify`), so
re-initializing with identical data is silent.

> **Note** — `Name` and `ManufacturerName` are provided in addition to the
> originally requested fields (`type`, `model_id`, `product_name`,
> `product_archetype`, `certified`, `software_version`) because they live in
> the same payload and are commonly needed.

### 3.3 Owned lights and sensors

```cpp
const std::vector<std::unique_ptr<Light>>&  getLights()  const;
const std::vector<std::unique_ptr<Sensor>>& getSensors() const;
```

Both return a `const` reference to the device's internal list — no copy, no
ownership transfer. The contained objects are fully reactive `Light` / `Sensor`
instances (the same ones `Bridge::getLights()` / `Bridge::getSensors()`
produce), so you can read their properties and subscribe to their events:

```cpp
for (const auto& light : device->getLights()) {
    light->PropertyChanged += [](auto&, auto args) { /* react */ };
    bool on = light->IsOn;   // cache-first read
}
```

---

## 4. Bridge additions

```cpp
std::vector<std::unique_ptr<Device>> Bridge::getDevices();
std::unique_ptr<Device>              Bridge::getDevice(const std::string& device_id);
```

| Method | Returns | On failure |
|--------|---------|------------|
| `getDevices()` | All devices on the bridge, each populated with its lights/sensors | Empty vector (not authenticated, no IP, network/parse error) |
| `getDevice(id)` | The single matching device | `nullptr` (not found, not authenticated, empty id, error) |

Both require the bridge to be authenticated (`isAuthenticated()`), mirroring the
behavior of `getLights()` / `getSensors()`.

---

## 5. Behavior & semantics

### 5.1 How the owned lists are populated

`getDevices()` / `getDevice()` delegate to a shared private builder,
`Bridge::buildDevicesFromData()`, which:

1. **Fetches once.** Retrieves *all* lights (`getLights()`) and *all* sensors
   (`getSensors()`) and indexes them in `std::map<id, unique_ptr<…>>`.
2. **Walks each device's `services` array, in order.** For every service it
   looks up the referenced `rid` in the light index, then the sensor index, and
   moves the matching object into the device's list. The entry is erased from
   the index once claimed, so each light/sensor is owned by exactly one device.
3. **Skips what it does not model.** Resource ids are globally unique, so a
   service of a type hue4cpp does not represent (`zigbee_connectivity`,
   `device_power`, `device_software_update`, `entertainment`, …) simply matches
   neither index and is ignored.
4. **Releases the remainder.** Any light or sensor not owned by a returned
   device is destroyed when the indexes go out of scope.

```
            /clip/v2/resource/device         /resource/light   /resource/<sensor types>
                     │                              │                    │
                     ▼                              ▼                    ▼
              [ DeviceGet … ]                 light_by_id[]        sensor_by_id[]
                     │                              │                    │
                     └────────── for each device, walk services[] in order
                                          │
                            rid ∈ light_by_id ?  → device.addLight(move)
                            rid ∈ sensor_by_id ? → device.addSensor(move)
                                          │
                                          ▼
                              Device { lights[], sensors[] }
```

### 5.2 Ordering guarantee

The light and sensor lists follow the **device's declared `services` order**.
Because the bridge returns services in a stable order and ids are unique, the
ordering is deterministic and identical across repeated calls — and identical
between `getDevices()` and `getDevice()` since both use the same builder. This
preserves semantic order (e.g. buttons 1–4 of a dimmer switch stay in order)
rather than an arbitrary id sort.

### 5.3 Reactivity

`Device` itself does **not** subscribe to the bridge SSE stream; its metadata is
captured at fetch time. The lights and sensors it owns *are* reactive on their
own, exactly as before. (Device-level live updates — e.g. a metadata rename or
firmware-update progress pushed over SSE — are out of scope for this release;
see §9.)

### 5.4 Ownership & memory model

`Device`, like `Light` and `Sensor`, is an `ObservableObject` whose properties
capture `this`, so it is non-copyable and non-movable and is always handed out
as `std::unique_ptr`. It owns its lights/sensors via
`std::vector<std::unique_ptr<…>>`; destroying the device destroys them. The
destructor is defined out-of-line (in `device.cpp`) so the owned objects' types
are complete at the point of destruction.

---

## 6. API data mapping

The `Device` is built from an API `DeviceGet` object. Fields consumed:

| DeviceGet path | Mapped to |
|----------------|-----------|
| `id` | `Device::Id` |
| `type` | `Device::Type` |
| `metadata.name` | `Device::Name` |
| `product_data.model_id` | `Device::ModelId` |
| `product_data.manufacturer_name` | `Device::ManufacturerName` |
| `product_data.product_name` | `Device::ProductName` |
| `product_data.product_archetype` | `Device::ProductArchetype` |
| `product_data.certified` | `Device::Certified` |
| `product_data.software_version` | `Device::SoftwareVersion` |
| `services[].rid` / `services[].rtype` | routed into `getLights()` / `getSensors()` |

Fields **not** currently surfaced: `id_v1`, `product_data.hardware_platform_type`,
`metadata.archetype`, `identify`, `usertest`, `device_mode` (deprecated), and
`geometry`. Parsing is defensive — missing or partial blocks leave the
corresponding properties at their defaults and never throw.

---

## 7. Usage

### 7.1 List every device

```cpp
#include <hue4cpp/hue4cpp.h>
#include <iostream>
using namespace hue4cpp;

auto bridges = Bridge::discover();
auto& bridge = bridges.at(0);
bridge.authenticate("my-app");           // or setAuthenticationKey(savedKey)

for (const auto& device : bridge.getDevices()) {
    std::cout << device->Name.Get()
              << " — " << device->ProductName.Get()
              << " (" << device->ModelId.Get() << ", sw " << device->SoftwareVersion.Get() << ")\n"
              << "   archetype: " << device->ProductArchetype.Get()
              << ", certified: " << (device->Certified.Get() ? "yes" : "no") << "\n"
              << "   lights:  " << device->getLights().size()
              << ", sensors: " << device->getSensors().size() << "\n";

    for (const auto& light  : device->getLights())  std::cout << "     light  " << light->Id.Get()  << "\n";
    for (const auto& sensor : device->getSensors()) std::cout << "     sensor " << sensor->getId() << "\n";
}
```

### 7.2 Fetch one device by id

```cpp
auto device = bridge.getDevice("3a5c2d9e-0000-4000-8000-000000000001");
if (device) {
    std::cout << "Found: " << device->Name.Get() << std::endl;
}
```

A runnable program is provided in
[`examples/device_listing.cpp`](../examples/device_listing.cpp); it reuses the
authentication-key save/load mechanism from `sensor_monitoring` (key persisted
to `hue_auth_key.txt`, keyed by bridge id).

---

## 8. Performance considerations

Building devices fetches the full light list (1 request) and the full sensor
list (one request per sensor type — 9 today), in addition to the device list:

| Call | HTTP GETs | Notes |
|------|-----------|-------|
| `getDevices()` | `1 (device) + 1 (light) + 9 (sensors)` = **11** | Independent of device count |
| `getDevice(id)` | **11** | Shares the same builder for ordering consistency |

`getDevice(id)` therefore performs the same fan-out as `getDevices()` even
though it returns one device. This is a deliberate trade-off that keeps the
ordering logic identical between the two entry points. If single-device latency
matters, a future optimization can fetch only the device's referenced services
(see §9).

---

## 9. Limitations & future work

- **No device-level SSE reactivity.** `Device` metadata is a snapshot; it does
  not update live. (Owned lights/sensors remain reactive.)
- **No `getDeviceState` / `refresh()`.** Unlike `Light`/`Sensor`, `Device` does
  not cache its JSON in `StateManager` and offers no refresh helper yet.
- **`getDevice(id)` is not request-minimal** — see §8.
- **Unmodeled metadata.** `geometry`, `identify`, `usertest`, `device_mode`,
  `hardware_platform_type`, `metadata.archetype`, and `id_v1` are not exposed.
- **Service types beyond lights/sensors** (connectivity, power, software-update,
  entertainment, …) are not represented as objects; they are skipped when
  building the owned lists.

---

## 10. Testing

- `tests/device_tests.cpp` — 4 Catch2 test cases / 27 assertions covering
  construction, full and partial `initFromJson` parsing (using the API doc's
  example payload), empty-list defaults, and the `PropertyChanged` /
  equality-guard behavior.
- The full suite was verified with a stash-and-rebuild baseline: the change adds
  exactly the 4 new (passing) Device cases and introduces **zero regressions**.
- Builds cleanly under MSVC `/W4` with no new warnings.

> Integration of `getDevices()` / `getDevice()` against a live bridge is not
> unit-tested (consistent with the existing `getLights()` / `getSensors()`
> network paths, which require a real bridge).

---

## 11. Files changed

**Added**

- `include/hue4cpp/device.h` — `Device` class
- `src/device.cpp` — implementation
- `tests/device_tests.cpp` — unit tests
- `examples/device_listing.cpp` — example program

**Modified**

- `include/hue4cpp/bridge.h` — `getDevices()` / `getDevice()` declarations,
  `buildDevicesFromData()` helper, `Device` forward declaration,
  `<nlohmann/json_fwd.hpp>` include
- `src/bridge.cpp` — method implementations and a file-local
  `fetchResourceData()` helper
- `include/hue4cpp/hue4cpp.h` — include `device.h`
- `src/CMakeLists.txt` — build `device.cpp`
- `tests/CMakeLists.txt` — build `device_tests.cpp`
- `examples/CMakeLists.txt` — build `device_listing`
- `README.md` — feature bullet, usage section, structure entries

---

## 12. Compatibility

Fully backward compatible. No existing class, method signature, or behavior was
changed; all additions are new symbols. Existing code continues to compile and
run unchanged.
