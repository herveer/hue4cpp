# Library source files
set(HUE4CPP_SOURCES
    bridge.cpp
    light.cpp
    device.cpp
    state.cpp
    http_client.cpp
    json_utils.cpp
    discovery.cpp
    color_utils.cpp
    sse_client.cpp
    sensors/sensor_base.cpp
    sensors/motion_sensor.cpp
    sensors/temperature_sensor.cpp
    sensors/light_level_sensor.cpp
    sensors/button_sensor.cpp
    sensors/camera_motion_sensor.cpp
    sensors/bell_button_sensor.cpp
    sensors/relative_rotary_sensor.cpp
    sensors/geolocation_sensor.cpp
    sensors/tamper_sensor.cpp
)

# Create the library
add_library(hue4cpp ${HUE4CPP_SOURCES})

# Set library properties
set_target_properties(hue4cpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(hue4cpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MDNS_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(hue4cpp
    PUBLIC
        cpr::cpr
        nlohmann_json::nlohmann_json
        ReactiveLitepp::ReactiveLitepp
)

if (WIN32)
    target_link_libraries(hue4cpp PUBLIC ws2_32)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(hue4cpp PRIVATE /W4)
else()
    target_compile_options(hue4cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()
