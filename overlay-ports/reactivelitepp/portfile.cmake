vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO herve-er/ReactiveLitepp
    REF "v${VERSION}"
    SHA512 8aec3039330e421872025a8af5584c10a068e2fb0148def95592ef4470b722d015b75f56e1c1de31195678cd749fa23b5f483cd53683d32354ce13a2937a96e8
    HEAD_REF main
)

# Fix case mismatch: the config template is lowercase but CMakeLists.txt expects PascalCase
file(COPY "${SOURCE_PATH}/cmake/reactivelitepp-config.cmake.in"
     DESTINATION "${SOURCE_PATH}/cmake")
file(RENAME "${SOURCE_PATH}/cmake/reactivelitepp-config.cmake.in"
            "${SOURCE_PATH}/cmake/ReactiveLitepp-config.cmake.in")

set(VCPKG_BUILD_TYPE release)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${SOURCE_PATH}/README.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")