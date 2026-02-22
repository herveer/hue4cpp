# GitHub Copilot Instructions for hue4cpp

## Repository Overview

**hue4cpp** is a modern, lightweight C++ library providing a developer-friendly interface to the Philips Hue V2 API. The library enables cross-platform control of Philips Hue smart lighting systems with features including light control, automatic bridge discovery, and real-time state synchronization.

**Current Status**: Phase 0 - Foundation Complete ✅, Phase 1 - Core Infrastructure in progress 🚧

## Project Structure

```
hue4cpp/
├── include/hue4cpp/      # Public API headers
│   ├── hue4cpp.h         # Main header (include this for all functionality)
│   ├── bridge.h          # Bridge discovery and connection
│   ├── light.h           # Light control
│   ├── state.h           # State management
│   ├── types.h           # Common types
│   ├── exceptions.h      # Exception types
│   ├── http_client.h     # HTTP client wrapper
│   └── json_utils.h      # JSON parsing utilities
├── src/                  # Implementation files (.cpp)
├── tests/                # Unit tests (Catch2 framework)
├── examples/             # Example applications
├── doc/                  # Documentation
└── cmake/                # CMake configuration files
```

## Technology Stack

- **Language**: C++17 (minimum requirement)
- **Build System**: CMake 3.16+
- **Package Manager**: vcpkg
- **Dependencies**:
  - `nlohmann-json`: JSON parsing and serialization
  - `cpr`: HTTP client library (built on libcurl)
  - `Catch2` (v3): Unit testing framework

## Building and Testing

### Build Commands

```bash
# Standard build with vcpkg
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### Build Options

- `BUILD_TESTS=ON/OFF` - Build unit tests (default: ON)
- `BUILD_EXAMPLES=ON/OFF` - Build example applications (default: ON)
- `BUILD_SHARED_LIBS=ON/OFF` - Build as shared library (default: OFF)

### Running Tests

```bash
# From build directory
ctest --output-on-failure

# Or run test executable directly
./tests/hue4cpp_tests
```

### Test Coverage

- All new features must include comprehensive unit tests
- Use Catch2 test framework with descriptive test names
- Organize tests with `TEST_CASE` and `SECTION` blocks
- Target test coverage: >90%

## Code Style and Conventions

### File Organization

- **Header files**: Use `#pragma once` for include guards
- **Public headers**: Place in `include/hue4cpp/`
- **Implementation files**: Place in `src/`
- **Test files**: Place in `tests/` with `_tests.cpp` suffix

### Naming Conventions

- **Classes**: PascalCase (e.g., `Bridge`, `StateManager`)
- **Functions/Methods**: camelCase (e.g., `discover()`, `getLights()`)
- **Variables**: snake_case (e.g., `auth_key`, `bridge_info`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAJOR`, `MINOR`)
- **Namespaces**: lowercase (e.g., `hue4cpp`, `json_utils`)
- **Private members**: No specific prefix, use clear naming

### Code Structure

- **Pimpl Idiom**: Use for classes with complex private state (see `Bridge` class)
- **RAII**: Resource management through C++ objects
- **Value Semantics**: Prefer value types where appropriate
- **Move Semantics**: Delete copy constructors/operators, implement move operations for resource-owning classes
- **Exception Safety**: Provide strong exception safety guarantees
- **const Correctness**: Mark methods const when they don't modify object state

### Documentation

- Use Doxygen-style comments for all public APIs
- Include `@brief`, `@param`, `@return`, `@throws` tags as appropriate
- Document file purpose with `@file` and `@brief` at the top
- Keep comments clear, concise, and up-to-date with code changes

Example:
```cpp
/**
 * @brief Discover Hue bridges on the local network
 * @return Vector of discovered bridges
 * @throws NetworkException if discovery fails
 */
static std::vector<Bridge> discover();
```

### Header Structure

```cpp
#pragma once

#include <system_headers>  // Standard library headers
#include "local_headers.h" // Project headers

/**
 * @file filename.h
 * @brief Brief description
 */

namespace hue4cpp {

// Forward declarations
class ClassName;

// Class/function definitions

} // namespace hue4cpp
```

### Error Handling

- **Use custom exception types** from `exceptions.h` for:
  - Network failures (connection errors, timeouts)
  - Invalid API responses or malformed data
  - Authentication failures
  - Resource not found errors
- **Use `std::optional`** for operations that may legitimately not return a value:
  - Optional configuration values
  - Queries that may not find results
  - Feature detection (capabilities that may not be present)
- **Provide clear, actionable error messages** that help users understand what went wrong and how to fix it

### Compiler Warnings

- MSVC: `/W4` warning level
- GCC/Clang: `-Wall -Wextra -Wpedantic`
- **All warnings must be addressed** - no warning suppressions without justification

## Development Workflow

### Before Making Changes

1. Review `ROADMAP.md` to understand current development phase and priorities
2. Check existing tests to understand expected behavior
3. Ensure changes align with project architecture and design principles

### Making Changes

1. **Small, focused changes**: Each PR should address a single concern
2. **Tests first**: Write or update tests before implementation when possible
3. **Maintain backward compatibility**: Don't break existing public APIs
4. **Update documentation**: Keep README, ROADMAP, and code comments in sync

### Testing Your Changes

1. Build the project with your changes
2. Run all unit tests: `ctest --output-on-failure`
3. Build and run examples to verify functionality
4. Test on multiple platforms if possible (Windows, Linux, macOS)

### Before Committing

- Ensure all tests pass
- Verify no new compiler warnings
- Check that documentation is updated
- Review your changes for unnecessary modifications

## Architecture and Design Principles

### Design Principles

- **RAII**: Automatic resource management
- **Value Semantics**: Prefer value types where appropriate
- **Exception Safety**: Strong exception guarantees
- **Thread Safety**: Design thread-safe public APIs where needed
- **Zero-Cost Abstractions**: Minimize runtime overhead

### Key Components

```
Application Layer
      ↓
hue4cpp API (Bridge, Light, State, Events)
      ↓
Core Services (HTTP, JSON, SSE, Discovery)
      ↓
External Dependencies (cpr, nlohmann-json, Catch2)
```

### Current Implementation Status

- ✅ Project structure and build system
- ✅ Basic HTTP client and JSON utilities
- ✅ Exception framework
- 🚧 Bridge discovery (in progress)
- 🚧 Authentication (planned)
- ⏳ Light control (planned)
- ⏳ State management and SSE (planned)

## Security and Best Practices

### Security Considerations

- **HTTPS Required**: All Hue API V2 communication must use HTTPS
- **Certificate Validation**: Implement proper SSL/TLS certificate validation
- **Credential Storage**: Use OS keychain integration for secure key storage (future)
- **Input Validation**: Validate all user input and API responses
- **No Hardcoded Secrets**: Never commit authentication keys or credentials

### Performance Considerations

- Target < 100ms for basic operations
- Use move semantics to avoid unnecessary copies
- Prefer stack allocation over heap when possible
- Minimize dependencies in public headers

### Cross-Platform Support

- Avoid platform-specific APIs in core library
- Use CMake for portable build configuration
- Test on all major platforms: Windows, Linux, macOS
- Use vcpkg for consistent dependency management

### API Usage Guidelines

- **Rate Limiting**: The Philips Hue API has rate limits; design for reasonable request frequency
- **Connection Management**: Reuse connections when possible to minimize overhead
- **Event Streaming**: Use SSE (Server-Sent Events) for real-time updates instead of polling
- **Graceful Degradation**: Handle API unavailability gracefully with appropriate error messages

## Common Tasks

### Adding a New Feature

1. Check `ROADMAP.md` for planned features and priorities
2. Design API to match existing patterns (see `Bridge` or `Light` classes)
3. Add public header to `include/hue4cpp/`
4. Add implementation to `src/`
5. Add comprehensive unit tests to `tests/`
6. Update `README.md` if it affects public API
7. Add usage example to `examples/` directory

### Adding Dependencies

1. Add to `vcpkg.json` dependencies array
2. Update `CMakeLists.txt` with `find_package()` and `target_link_libraries()`
3. Document in README.md
4. Ensure dependency is available on all platforms

### Writing Tests

Use Catch2 framework with clear test structure:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "hue4cpp/your_header.h"

using namespace hue4cpp;

TEST_CASE("Feature description", "[tag]") {
    SECTION("Specific scenario") {
        // Arrange
        auto object = YourClass();
        
        // Act
        auto result = object.method();
        
        // Assert
        REQUIRE(result == expected_value);
    }
}
```

### Debugging Build Issues

- Ensure vcpkg is properly configured and integrated
- Check CMake cache: delete `build/` and reconfigure
- Verify all dependencies are installed via vcpkg
- Check compiler version meets C++17 requirements

## Important Files

- `CMakeLists.txt`: Root build configuration
- `vcpkg.json`: Dependency manifest
- `ROADMAP.md`: Development plan and next steps
- `README.md`: User-facing documentation
- `.gitignore`: Excludes build artifacts, IDE files

## Contribution Guidelines

1. **Code Quality**: Follow existing code style and conventions
2. **Testing**: All new code must have unit tests
3. **Documentation**: Update README and inline documentation
4. **Commits**: Use clear, descriptive commit messages
5. **Pull Requests**: Keep focused and well-documented

## Additional Notes

- This library is **not affiliated with or endorsed by Signify (Philips Hue)**
- All development should follow modern C++ best practices
- Favor readability and maintainability over premature optimization
- When in doubt, check existing code for patterns and conventions

## Light Class API Changes

- The Light class API has been refactored to expose only observable Properties as the public interface.
- All control methods (e.g., `turnOn`, `setBrightness`, `getColor`, `setColor`, `toggle`, `alert`, etc.) are now private and only called from within the Properties.
- This design encourages the use of reactive properties for state management and provides a cleaner, more consistent API.
