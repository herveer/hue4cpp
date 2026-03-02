# Project Overview

**hue4cpp** is a modern, lightweight C++17 library that provides a
developer-friendly interface to the Philips Hue V2 API. It enables
cross-platform control of Philips Hue smart lighting systems with
automatic bridge discovery (mDNS & N-UPnP), secure authentication,
comprehensive light and sensor control, real-time state
synchronization via Server-Sent Events (SSE), and advanced color
management (RGB, XY, HSV, color-temperature). The library targets
Windows, Linux, and macOS and is distributed under the MIT License.

## Repository Structure

- `include/hue4cpp/` — Public API headers consumed by downstream
  users; single-entry point via `hue4cpp.h`.
  - `sensors/` — Per-type sensor headers (motion, temperature,
    light-level, button, camera-motion, bell-button, rotary,
    geolocation, tamper).
- `src/` — Library implementation files; mirrors the public header
  structure plus `sensors/` sub-directory.
- `tests/` — Catch2 v3 unit tests; one `*_tests.cpp` file per module.
- `examples/` — Standalone example programs demonstrating discovery,
  authentication, light/color control, SSE monitoring, sensors,
  and performance benchmarks.
- `doc/` — Documentation root; `hueApiV2/` contains Philips Hue API
  reference materials; `generated/` (gitignored) holds Doxygen output.
- `cmake/` — CMake package-config template
  (`hue4cppConfig.cmake.in`).
- `.github/` — GitHub config: Copilot instructions
  (`copilot-instructions.md`) and CI workflows (`workflows/`).

## Build & Development Commands

### Prerequisites

| Tool      | Minimum Version |
|-----------|-----------------|
| C++ compiler (GCC / Clang / MSVC) | GCC 8+ / Clang 7+ / MSVC 2019+ |
| CMake     | 3.16            |
| vcpkg     | latest          |
| Doxygen   | any (docs only) |

### Install dependencies (vcpkg)

```bash
# Clone vcpkg if not already present
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh        # Windows: bootstrap-vcpkg.bat
./vcpkg integrate install
```

### Configure & build

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### Build options

```bash
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=[vcpkg-path] \
  -DBUILD_TESTS=ON \          # default ON
  -DBUILD_EXAMPLES=ON \       # default ON
  -DBUILD_SHARED_LIBS=OFF     # default OFF
```

### Run tests

```bash
cd build
ctest --output-on-failure
# Or run the binary directly:
./tests/hue4cpp_tests
```

### Generate API docs

```bash
doxygen Doxyfile
# Output: doc/generated/html/index.html
```

### Lint / static analysis

> TODO: No linter or static-analysis configuration is currently
> committed. Consider adding `clang-tidy` or `cppcheck` targets.

## Code Style & Conventions

### Naming

| Element        | Convention          | Example                     |
|----------------|---------------------|-----------------------------|
| Classes        | PascalCase          | `Bridge`, `StateManager`    |
| Methods        | camelCase           | `discover()`, `getLights()` |
| Variables      | snake_case          | `auth_key`, `bridge_info`   |
| Constants      | UPPER_SNAKE_CASE    | `MAJOR`, `MINOR`            |
| Namespaces     | lowercase           | `hue4cpp`, `json_utils`     |

### Formatting & header rules

- Use `#pragma once` for include guards.
- System headers first (`<vector>`, `<string>`), then project
  headers (`"hue4cpp/bridge.h"`).
- Prefer Pimpl for classes with complex private state.
- Delete copy constructors where move semantics are used.
- Mark read-only methods `const`.
- Use Doxygen-style comments (`@brief`, `@param`, `@return`,
  `@throws`) for every public symbol.
- Each file starts with a `@file` / `@brief` block.

### Compiler warnings (enforced)

- MSVC: `/W4`
- GCC / Clang: `-Wall -Wextra -Wpedantic`
- All warnings must compile cleanly — no suppressions without
  written justification.

### Commit messages

Use clear, descriptive commit messages. Include the module or area
affected (e.g., `[bridge] Add mDNS retry logic`).

## Architecture Notes

```
┌───────────────────────────────────────────────────┐
│                  User Application                 │
│            #include <hue4cpp/hue4cpp.h>           │
└──────────────────────┬────────────────────────────┘
                       │
          ┌────────────▼────────────┐
          │         Bridge          │
          │  discover() / auth()    │
          │  getLights/getSensors   │
          └────┬──────────┬────────┘
               │          │
   ┌───────────▼──┐  ┌────▼───────────┐
   │   Light /    │  │  StateManager  │
   │   Sensors    │  │  (SSE client)  │
   │   control    │  │  callbacks     │
   └──────┬───────┘  └──────┬─────────┘
          │                 │
   ┌──────▼─────────────────▼──────────┐
   │           HttpClient              │
   │      (cpr / libcurl wrapper)      │
   └──────────────┬────────────────────┘
                  │
   ┌──────────────▼──────────��─────────┐
   │         json_utils                │
   │    (nlohmann-json helpers)        │
   └───────────────────────────────────┘
```

**Key data flow:**

1. `Bridge::discover()` uses mDNS (`mdns.h` header-only) and/or
   N-UPnP (HTTPS via `cpr`) to locate bridges on the LAN.
2. `Bridge::authenticate()` performs the Hue link-button flow and
   stores the application key.
3. Light and sensor objects are obtained from the bridge; every
   command is translated to a Hue V2 REST call via `HttpClient`.
4. `StateManager` opens an SSE connection to the bridge's
   `/eventstream` endpoint, parses events, and dispatches them
   through an observer-pattern callback system backed by
   `ReactiveLitepp`.
5. Color conversions (RGB ↔ XY, HSV ↔ RGB, Kelvin ↔ mireds) are
   handled by `color_utils`.

## Testing Strategy

### Unit tests

- **Framework**: Catch2 v3 (`Catch2::Catch2WithMain`).
- **Location**: `tests/` — one file per module (e.g.,
  `bridge_tests.cpp`, `light_tests.cpp`, `sensor_tests.cpp`).
- **Style**: use `TEST_CASE` with `SECTION` blocks; descriptive
  names in both.
- **Coverage target**: > 90 %.
- **Run locally**:

  ```bash
  cd build
  ctest --output-on-failure
  ```

### Integration / end-to-end

> TODO: No integration-test infrastructure is currently present.
> Tests that require an actual Hue bridge are not automated.

### CI

- A GitHub Actions workflow (`generate-docs.yml`) automatically
  generates and publishes Doxygen documentation on pushes to
  `main` or `release/*` branches when source or Doxyfile changes.

> TODO: No CI workflow for build + test across platforms exists
> yet. Consider adding a matrix build workflow.

## Security & Compliance

- **Secrets handling**: Hue bridge application keys should never be
  committed. The library supports storing keys at runtime; the
  authentication flow generates keys interactively.
- **TLS**: The Hue V2 API requires HTTPS. `HttpClient` wraps `cpr`
  which uses libcurl; SSL verification can be toggled via
  `setVerifySsl()`. Production usage should always keep it enabled.
- **Dependency scanning**: Dependencies are pinned via
  `vcpkg-configuration.json` baselines. A custom vcpkg registry
  (`herve-er/vcpkg-registery`) supplies `reactivelitepp`.

> TODO: No automated dependency-scanning (e.g., Dependabot,
> `ossf/scorecard`) is configured.

- **License**: MIT (see `LICENSE`).

## Agent Guardrails

1. **Never modify** the following files without explicit human
   review:
   - `LICENSE`
   - `vcpkg-configuration.json` (registry baselines)
   - `.github/workflows/*.yml`
2. **Always run the full test suite** (`ctest --output-on-failure`)
   before proposing any code change.
3. **Do not add new vcpkg dependencies** without explicit approval;
   update `vcpkg.json` and `vcpkg-configuration.json` together.
4. **Do not commit generated files** — `doc/generated/` is
   gitignored; Doxygen output must be produced by CI.
5. **Respect warning policy** — a PR must compile cleanly with
   `/W4` (MSVC) or `-Wall -Wextra -Wpedantic` (GCC/Clang).
6. **Every new public API** must include Doxygen documentation and
   at least one Catch2 test case.
7. **Do not store bridge credentials** or real IP addresses in
   source, test fixtures, or examples.

## Extensibility Hooks

- **CMake build options**: `BUILD_TESTS`, `BUILD_EXAMPLES`,
  `BUILD_SHARED_LIBS` control what gets compiled.
- **Custom vcpkg registry**: `reactivelitepp` is provided through
  a private registry declared in `vcpkg-configuration.json`.
- **Sensor hierarchy**: New sensor types can be added by creating
  a header in `include/hue4cpp/sensors/`, an implementation in
  `src/sensors/`, and registering the source in
  `src/CMakeLists.txt`.
- **Event system**: `StateManager` callbacks and `SSEClient` events
  use an observer pattern (`ReactiveLitepp`); consumers subscribe
  with lambdas.
- **Color presets**: Extend preset colors in `color_utils.h` /
  `color_utils.cpp`.

> TODO: No runtime feature flags or environment variables are
> currently used.

## Further Reading

- [README.md](README.md) — Quick-start guide and API examples.
- [ROADMAP.md](ROADMAP.md) — Development phases and upcoming
  features.
- [doc/README.md](doc/README.md) — How to generate and browse
  Doxygen documentation.
- [doc/hueApiV2/](doc/hueApiV2/) — Philips Hue V2 API reference
  materials.
- [.github/copilot-instructions.md](.github/copilot-instructions.md)
  — Copilot-specific coding conventions for this repository.