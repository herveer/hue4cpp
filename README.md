# hue4cpp

A modern, lightweight C++ library providing a developer-friendly interface to the Philips Hue V2 API.

## Overview

**hue4cpp** is a cross-platform C++ library designed to make controlling Philips Hue smart lighting systems simple and intuitive. Built with modern C++ practices, this library offers a clean API for discovering bridges, controlling lights, and maintaining real-time state synchronization.

## Features

- 🔍 **Automatic Bridge Discovery**: Find Hue bridges on your network automatically (mDNS and N-UPnP)
- 🔐 **Secure Authentication**: Interactive authentication flow with key storage
- ✨ **Comprehensive Light Control**: Complete control of individual lights
  - Power: On/off with smooth transitions
  - Brightness: 0-100% dimming control
  - Color: RGB/XY color space with automatic conversion
  - Color Temperature: Kelvin/mireds support (2000K-6500K)
  - Preset Colors: Built-in color palette
  - Effects: Alert/notification patterns
- 📊 **Sensor Support**: Read and monitor all sensor types
  - Motion Sensors: Detect motion events
  - Temperature Sensors: Monitor ambient temperature (°C)
  - Light Level Sensors: Track illuminance levels
  - Button Sensors: Capture button press events (dimmer switches, tap dials)
- 🎨 **Advanced Color Support**: 
  - RGB ↔ XY color space conversion
  - HSV ↔ RGB color conversion
  - 17+ preset colors (Red, Blue, WarmWhite, etc.)
  - Custom color temperature presets
- 🔄 **Real-time State Updates**: Server-Sent Events (SSE) for instant state synchronization
- 📡 **Event Callbacks**: Observer pattern for light, sensor, and bridge state changes
- 🔧 **Capability-aware**: Automatically detects and adapts to device capabilities
- 🚀 **Lightweight**: Minimal dependencies, fast and efficient
- 🔧 **Cross-platform**: Works on Windows, Linux, and macOS
- 📦 **Easy Integration**: CMake build system with vcpkg dependency management
- 📖 **Clean API**: Ultra-readable, well-documented code

## Project Status

🚧 **Under Active Development** 🚧

This project is currently in its initial development phase. See [ROADMAP.md](ROADMAP.md) for the development plan and upcoming features.

## Requirements

- **C++ Compiler**: C++17 or later
  - GCC 8+
  - Clang 7+
  - MSVC 2019+
- **CMake**: 3.16 or later
- **vcpkg**: For dependency management

## Dependencies

All dependencies are managed via vcpkg:

- **nlohmann-json**: JSON parsing and serialization
- **cpr**: HTTP client library (built on libcurl)
- **Catch2**: Unit testing framework

## Quick Start

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/herve-er/hue4cpp.git
   cd hue4cpp
   ```

2. **Set up vcpkg** (if not already installed):
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # Use bootstrap-vcpkg.bat on Windows
   ./vcpkg integrate install
   ```

3. **Build the library**:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build .
   ```

4. **Run tests**:
   ```bash
   ctest --output-on-failure
   ```

### Usage Example

```cpp
#include <hue4cpp/hue4cpp.h>

int main() {
    using namespace hue4cpp;
    
    // Discover bridges on the network
    auto bridges = Bridge::discover();
    
    if (bridges.empty()) {
        std::cerr << "No bridges found!" << std::endl;
        return 1;
    }
    
    // Connect to the first bridge
    auto& bridge = bridges[0];
    
    // Authenticate with the bridge (press the button first!)
    auto auth_result = bridge.authenticate("your-app-name");
    if (!auth_result.isSuccess()) {
        std::cerr << "Authentication failed!" << std::endl;
        return 1;
    }
    
    // Get all lights
    auto lights = bridge.getLights();
    
    // Control the first light
    if (!lights.empty()) {
        auto& light = lights[0];
        
        // Turn on with smooth transition
        light.turnOn(std::chrono::milliseconds(1000));
        
        // Set brightness to 75%
        light.setBrightness(75);
        
        // Set color using RGB
        light.setColor(colors::Blue);  // Use preset color
        // Or use custom RGB values
        light.setColor(RGBColor(255, 0, 128));
        
        // Set color temperature (warm white)
        auto temp = ColorTemperature::fromKelvin(2700);
        light.setColorTemperature(temp);
    }
    
    return 0;
}
```

### Real-time State Monitoring

Monitor light state changes in real-time using Server-Sent Events:

```cpp
#include <hue4cpp/hue4cpp.h>
#include <iostream>

int main() {
    using namespace hue4cpp;
    
    // Set up bridge and authenticate...
    auto bridges = Bridge::discover();
    auto& bridge = bridges[0];
    bridge.authenticate("your-app-name");
    
    // Get the state manager
    auto& state_manager = bridge.getStateManager();
    
    // Register callback for state changes
    state_manager.registerCallback([](const Event& event) {
        switch (event.type) {
            case EventType::LightStateChanged:
                std::cout << "Light state changed: " << event.resource_id << std::endl;
                break;
            case EventType::BridgeConnected:
                std::cout << "Bridge connected!" << std::endl;
                break;
            case EventType::BridgeDisconnected:
                std::cout << "Bridge disconnected!" << std::endl;
                break;
            default:
                break;
        }
    });
    
    // Start real-time monitoring
    auto result = state_manager.start();
    if (result.isSuccess()) {
        std::cout << "Real-time monitoring started!" << std::endl;
        
        // Your application logic here...
        // Events will be delivered via callbacks
        
        // Stop monitoring when done
        state_manager.stop();
    }
    
    return 0;
}
```

### Sensor Monitoring

Access and monitor sensors (motion, temperature, light level, buttons):

```cpp
#include <hue4cpp/hue4cpp.h>
#include <iostream>

int main() {
    using namespace hue4cpp;
    
    // Set up bridge and authenticate...
    auto bridges = Bridge::discover();
    auto& bridge = bridges[0];
    bridge.authenticate("your-app-name");
    
    // Get all sensors
    auto sensors = bridge.getSensors();
    
    for (const auto& sensor : sensors) {
        std::cout << "Sensor: " << sensor.getId() << std::endl;
        
        // Check sensor type and read state
        if (sensor.getType() == SensorType::Motion) {
            auto state = sensor.getMotionState();
            if (state.has_value()) {
                std::cout << "  Motion: " << (state->motion ? "Detected" : "None") << std::endl;
            }
        }
        else if (sensor.getType() == SensorType::Temperature) {
            auto state = sensor.getTemperatureState();
            if (state.has_value()) {
                std::cout << "  Temperature: " << state->temperature << " °C" << std::endl;
            }
        }
        else if (sensor.getType() == SensorType::LightLevel) {
            auto state = sensor.getLightLevelState();
            if (state.has_value()) {
                std::cout << "  Light Level: " << state->light_level << std::endl;
            }
        }
        else if (sensor.getType() == SensorType::Button) {
            auto state = sensor.getButtonState();
            if (state.has_value()) {
                std::cout << "  Last Button Event: " << static_cast<int>(state->last_event) << std::endl;
            }
        }
    }
    
    // Get sensors by type
    auto motion_sensors = bridge.getMotionSensors();
    auto temp_sensors = bridge.getTemperatureSensors();
    auto light_sensors = bridge.getLightLevelSensors();
    auto button_sensors = bridge.getButtonSensors();
    
    // Monitor sensor events in real-time
    auto& state_manager = bridge.getStateManager();
    state_manager.registerCallback([](const Event& event) {
        if (event.type == EventType::SensorStateChanged) {
            std::cout << "Sensor state changed: " << event.resource_id << std::endl;
        }
    });
    state_manager.start();
    
    return 0;
}
```

For more examples, see the `examples/` directory:
- `basic_control.cpp` - Basic light control operations
- `color_control.cpp` - Advanced color control and effects
- `discovery.cpp` - Bridge discovery
- `authentication.cpp` - Authentication flow
- `state_monitoring.cpp` - Real-time state monitoring with SSE
- `sensor_monitoring.cpp` - Sensor discovery and real-time monitoring
- `performance_benchmark.cpp` - Performance benchmarks

## Building

### Standard Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### Build Options

- `BUILD_TESTS`: Build unit tests (default: ON)
- `BUILD_EXAMPLES`: Build example applications (default: ON)
- `BUILD_SHARED_LIBS`: Build as shared library (default: OFF)

Example:
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-path] -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
```

## Testing

The project uses Catch2 for unit testing:

```bash
cd build
ctest --output-on-failure
```

Or run the test executable directly:
```bash
./tests/hue4cpp_tests
```

## Documentation

- [Philips Hue API V2 Documentation](doc/hueApiV2/) - Complete API reference
- [ROADMAP.md](ROADMAP.md) - Development roadmap and upcoming features

## Project Structure

```
hue4cpp/
├── CMakeLists.txt          # Root CMake configuration
├── vcpkg.json              # Dependency manifest
├── README.md               # This file
├── ROADMAP.md              # Development plan
├── .gitignore              # Git ignore rules
├── include/                # Public headers
│   └── hue4cpp/
│       ├── hue4cpp.h       # Main header
│       ├── bridge.h        # Bridge discovery and connection
│       ├── light.h         # Light control
│       ├── color_utils.h   # Color conversion utilities
│       ├── state.h         # State management
│       ├── sse_client.h    # Server-Sent Events client
│       ├── types.h         # Common types
│       ├── http_client.h   # HTTP client wrapper
│       ├── json_utils.h    # JSON utilities
│       └── exceptions.h    # Exception types
├── src/                    # Implementation files
│   ├── CMakeLists.txt
│   ├── bridge.cpp
│   ├── light.cpp
│   ├── color_utils.cpp
│   ├── state.cpp
│   ├── sse_client.cpp
│   ├── http_client.cpp
│   ├── json_utils.cpp
│   └── discovery.cpp
├── tests/                  # Unit tests
│   ├── CMakeLists.txt
│   ├── bridge_tests.cpp
│   ├── light_tests.cpp
│   ├── color_utils_tests.cpp
│   ├── state_tests.cpp
│   ├── sse_client_tests.cpp
│   ├── http_client_tests.cpp
│   └── json_utils_tests.cpp
├── examples/               # Example applications
│   ├── CMakeLists.txt
│   ├── basic_control.cpp
│   ├── color_control.cpp
│   ├── state_monitoring.cpp
│   ├── performance_benchmark.cpp
│   ├── discovery.cpp
│   └── authentication.cpp
└── doc/                    # Documentation
    └── hueApiV2/           # API V2 documentation
```

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

### Development Guidelines

1. **Code Style**: Follow the existing code style (readable, well-commented)
2. **Testing**: Add tests for new features
3. **Documentation**: Update README and inline documentation
4. **Commits**: Use clear, descriptive commit messages

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Philips Hue for their comprehensive API documentation
- The C++ community for excellent tools and libraries

## Support

For issues, questions, or contributions, please use the [GitHub issue tracker](https://github.com/herve-er/hue4cpp/issues).

---

**Note**: This library is not affiliated with or endorsed by Signify (Philips Hue). All trademarks are property of their respective owners.
