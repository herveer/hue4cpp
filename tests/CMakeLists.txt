# Find Catch2
find_package(Catch2 3 CONFIG REQUIRED)

# Test sources
set(TEST_SOURCES
    main.cpp
    bridge_tests.cpp
    light_tests.cpp
    state_tests.cpp
    http_client_tests.cpp
    json_utils_tests.cpp
    exceptions_tests.cpp
    color_utils_tests.cpp
    sse_client_tests.cpp
)

# Create test executable
add_executable(hue4cpp_tests ${TEST_SOURCES})

# Link with library and Catch2
target_link_libraries(hue4cpp_tests
    PRIVATE
        hue4cpp
        Catch2::Catch2WithMain
)

# Set C++ standard
set_target_properties(hue4cpp_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include Catch2 CMake modules
include(CTest)
include(Catch)

# Register tests with CTest
catch_discover_tests(hue4cpp_tests)

# Add test target
add_test(NAME hue4cpp_tests COMMAND hue4cpp_tests)
