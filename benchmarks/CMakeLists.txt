project(benchmarks)
add_executable(${PROJECT_NAME} orderbook_benchmarks.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy benchmark::benchmark)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# Warnings for benchmarks (but not as strict to avoid benchmark library warnings)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wconversion -Wsign-conversion
    )
endif()

# Add BMI2 support for x86-64 platforms
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-mbmi2>
        $<$<CXX_COMPILER_ID:Clang>:-mbmi2>
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    )
endif()

add_executable(${PROJECT_NAME}_portable orderbook_benchmarks.cpp)

target_link_libraries(${PROJECT_NAME}_portable PRIVATE jazzy benchmark::benchmark)
target_compile_features(${PROJECT_NAME}_portable PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME}_portable PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# Warnings for portable benchmarks
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME}_portable PRIVATE
        -Wconversion -Wsign-conversion
    )
endif()

target_compile_definitions(${PROJECT_NAME}_portable PRIVATE
    JAZZY_HAS_BUILTIN_POPCOUNT=0
    JAZZY_HAS_BUILTIN_CTZ=0
    JAZZY_HAS_BUILTIN_CLZ=0
)
