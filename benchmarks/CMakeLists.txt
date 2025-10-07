project(benchmarks)
add_executable(${PROJECT_NAME} orderbook_benchmarks.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy benchmark::benchmark)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

# Warnings for benchmarks (treat warnings as errors)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Werror
        -Wconversion -Wsign-conversion
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /WX)
endif()

# Sanitizers for benchmarks (disabled by default for performance)
if(ENABLE_ASAN_BENCHMARKS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    endif()
endif()

if(ENABLE_UBSAN_BENCHMARKS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        # Clang-specific sanitizers (exclude integer for benchmarks as it can cause false positives)
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined,nullability,implicit-conversion,local-bounds,float-divide-by-zero -fno-sanitize-recover=all)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined,nullability,implicit-conversion,local-bounds,float-divide-by-zero)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC doesn't support nullability, implicit-conversion, local-bounds
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined,float-divide-by-zero -fno-sanitize-recover=all)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined,float-divide-by-zero)
    endif()
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

# Warnings for portable benchmarks (treat warnings as errors)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME}_portable PRIVATE
        -Werror
        -Wconversion -Wsign-conversion
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME}_portable PRIVATE /WX)
endif()

# Sanitizers for portable benchmarks (disabled by default for performance)
if(ENABLE_ASAN_BENCHMARKS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=address)
    endif()
endif()

if(ENABLE_UBSAN_BENCHMARKS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        # Clang-specific sanitizers (exclude integer for benchmarks as it can cause false positives)
        target_compile_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=undefined,nullability,implicit-conversion,local-bounds,float-divide-by-zero -fno-sanitize-recover=all)
        target_link_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=undefined,nullability,implicit-conversion,local-bounds,float-divide-by-zero)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC doesn't support nullability, implicit-conversion, local-bounds
        target_compile_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=undefined,float-divide-by-zero -fno-sanitize-recover=all)
        target_link_options(${PROJECT_NAME}_portable PRIVATE -fsanitize=undefined,float-divide-by-zero)
    endif()
endif()

target_compile_definitions(${PROJECT_NAME}_portable PRIVATE
    JAZZY_HAS_BUILTIN_POPCOUNT=0
    JAZZY_HAS_BUILTIN_CTZ=0
    JAZZY_HAS_BUILTIN_CLZ=0
)
