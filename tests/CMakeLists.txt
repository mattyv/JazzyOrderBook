project(tests)
add_executable(${PROJECT_NAME}
    main.cpp
    orderbook_tests.cpp
    select_nth_test.cpp
    out_of_range_tests.cpp
    tick_type_strong_tests.cpp
    best_price_regression_tests.cpp
    level_bitmap_tests.cpp
    level_bitmap_portable_tests.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Additional strict warnings for tests
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wconversion -Wsign-conversion -Wcast-align -Wformat=2
    )
endif()

add_test(NAME unit_tests COMMAND ${PROJECT_NAME})

# Add BMI2 support for x86-64 platforms
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-mbmi2>
        $<$<CXX_COMPILER_ID:Clang>:-mbmi2>
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    )
endif()
