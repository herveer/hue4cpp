# Library source files
set(HUE4CPP_SOURCES
    bridge.cpp
    light.cpp
    state.cpp
    http_client.cpp
    json_utils.cpp
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
        $<$<NOT:$<PLATFORM_ID:Windows>>:${MDNS_INCLUDE_DIRS}>
)

# Link dependencies
target_link_libraries(hue4cpp
    PUBLIC
        cpr::cpr
        nlohmann_json::nlohmann_json
)

# Platform-specific libraries for mDNS support
if(WIN32)
    # Windows uses Bonjour/DNS-SD (mdnsresponder)
    target_link_libraries(hue4cpp PRIVATE mdnsresponder::mdnsresponder)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(hue4cpp PRIVATE /W4)
else()
    target_compile_options(hue4cpp PRIVATE -Wall -Wextra -Wpedantic)
endif()
