# Benchmarks CMakeLists.txt

# Add Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/benchmark benchmark_build)

# Create benchmark executable
add_executable(jazzy_benchmarks
    select_nth_benchmark.cpp
)

target_link_libraries(jazzy_benchmarks
    PRIVATE
    benchmark::benchmark_main
)

target_include_directories(jazzy_benchmarks
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
)

target_compile_features(jazzy_benchmarks PRIVATE cxx_std_20)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(jazzy_benchmarks PRIVATE -O3 -march=native)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(jazzy_benchmarks PRIVATE /O2)
endif()