
cmake_minimum_required(VERSION 3.21)
project(jazzy)

enable_testing()

# Sanitizer options (separate controls for tests and benchmarks)
option(ENABLE_ASAN_TESTS "Enable Address Sanitizer for tests" ON)
option(ENABLE_UBSAN_TESTS "Enable Undefined Behavior Sanitizer for tests" ON)
option(ENABLE_ASAN_BENCHMARKS "Enable Address Sanitizer for benchmarks" OFF)
option(ENABLE_UBSAN_BENCHMARKS "Enable Undefined Behavior Sanitizer for benchmarks" OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

# Compiler warnings for the interface library
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} INTERFACE
        -Wall -Wextra -Wpedantic -Woverflow
        $<$<CXX_COMPILER_ID:GNU>:-Wshift-overflow=2>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} INTERFACE
        /W4 /w14062 /w14165
    )
endif()

add_subdirectory(external)
add_subdirectory(tests)
add_subdirectory(benchmarks)
