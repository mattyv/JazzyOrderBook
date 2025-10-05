# External dependencies

include(FetchContent)

# Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3  # Use latest stable version
)

# Configure benchmark options
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

if(ENABLE_UBSAN)
    set(_jazzy_ubsan_sanitizers "undefined,integer,nullability,implicit-conversion,local-bounds,float-divide-by-zero")
    foreach(_jazzy_target benchmark benchmark_main)
        if(TARGET ${_jazzy_target})
            target_compile_options(${_jazzy_target} PRIVATE "-fno-sanitize=${_jazzy_ubsan_sanitizers}")
            target_link_options(${_jazzy_target} PRIVATE "-fno-sanitize=${_jazzy_ubsan_sanitizers}")
        endif()
    endforeach()
endif()
