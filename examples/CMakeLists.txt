# Examples CMakeLists.txt

# Basic control example
add_executable(basic_control basic_control.cpp)
target_link_libraries(basic_control PRIVATE hue4cpp)
set_target_properties(basic_control PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Color control example
add_executable(color_control color_control.cpp)
target_link_libraries(color_control PRIVATE hue4cpp)
set_target_properties(color_control PROPERTIES
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

# Authentication example
add_executable(authentication authentication.cpp)
target_link_libraries(authentication PRIVATE hue4cpp)
set_target_properties(authentication PROPERTIES
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

# State monitoring example
add_executable(state_monitoring state_monitoring.cpp)
target_link_libraries(state_monitoring PRIVATE hue4cpp)
set_target_properties(state_monitoring PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Sensor monitoring example
add_executable(sensor_monitoring sensor_monitoring.cpp)
target_link_libraries(sensor_monitoring PRIVATE hue4cpp)
set_target_properties(sensor_monitoring PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Performance benchmark
add_executable(performance_benchmark performance_benchmark.cpp)
target_link_libraries(performance_benchmark PRIVATE hue4cpp)
set_target_properties(performance_benchmark PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Interactive demo
add_executable(interactive_demo interactive_demo.cpp)
target_link_libraries(interactive_demo PRIVATE hue4cpp)
set_target_properties(interactive_demo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
