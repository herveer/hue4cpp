vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO herve-er/ReactiveLitepp
    REF v1.1.1
    SHA512 8aec3039330e421872025a8af5584c10a068e2fb0148def95592ef4470b722d015b75f56e1c1de31195678cd749fa23b5f483cd53683d32354ce13a2937a96e8
    HEAD_REF master
)

# Fix case-sensitivity: CMakeLists.txt references ${PROJECT_NAME}-config.cmake.in
# which resolves to ReactiveLitepp-config.cmake.in, but the actual file on disk
# is reactivelitepp-config.cmake.in.
if(NOT EXISTS "${SOURCE_PATH}/cmake/ReactiveLitepp-config.cmake.in")
    file(COPY_FILE
         "${SOURCE_PATH}/cmake/reactivelitepp-config.cmake.in"
         "${SOURCE_PATH}/cmake/ReactiveLitepp-config.cmake.in")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ReactiveLitepp CONFIG_PATH share/ReactiveLitepp)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
