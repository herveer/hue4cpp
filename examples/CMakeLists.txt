# Examples CMakeLists.txt

# Basic control example
add_executable(basic_control basic_control.cpp)
target_link_libraries(basic_control PRIVATE hue4cpp)
set_target_properties(basic_control PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Discovery example
add_executable(discovery discovery.cpp)
target_link_libraries(discovery PRIVATE hue4cpp)
set_target_properties(discovery PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# HTTP request example
add_executable(http_request_example http_request_example.cpp)
target_link_libraries(http_request_example PRIVATE hue4cpp)
set_target_properties(http_request_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

