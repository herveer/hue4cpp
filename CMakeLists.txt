cmake_minimum_required(VERSION 3.16)

project(hue4cpp 
    VERSION 0.1.0
    DESCRIPTION "A modern C++ library for the Philips Hue V2 API"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Workaround: cmake config mode on case-sensitive filesystems only searches for
# <PackageName>Config.cmake or <lowercasepackagename>-config.cmake, but the
# installed file is ReactiveLitepp-config.cmake (mixed-case with hyphen).
set(_rlpp_dir "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/ReactiveLitepp")
if(EXISTS "${_rlpp_dir}/ReactiveLitepp-config.cmake"
   AND NOT EXISTS "${_rlpp_dir}/reactivelitepp-config.cmake")
    file(CREATE_LINK "${_rlpp_dir}/ReactiveLitepp-config.cmake"
         "${_rlpp_dir}/reactivelitepp-config.cmake" SYMBOLIC COPY_ON_ERROR)
endif()
unset(_rlpp_dir)

find_package(ReactiveLitepp CONFIG REQUIRED)

# mdns is header-only
find_path(MDNS_INCLUDE_DIRS "mdns.h")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add the library
add_subdirectory(src)

# Add tests if enabled
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Add examples if enabled
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS hue4cpp
    EXPORT hue4cppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/hue4cpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT hue4cppTargets
    FILE hue4cppTargets.cmake
    NAMESPACE hue4cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hue4cpp
)

# Generate and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hue4cppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/hue4cppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hue4cpp
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/hue4cppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hue4cppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hue4cppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hue4cpp
)

# Print configuration summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "========================================")
message(STATUS "")
