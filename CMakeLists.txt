
cmake_minimum_required(VERSION 3.21)
project(jazzy)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERFACE_CXX_STANDARD 20
    INTERFACE_CXX_STANDARD_REQUIRED ON
    INTERFACE_CXX_EXTENSIONS OFF
)

add_subdirectory(tests)

# Add benchmarks subdirectory
option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
