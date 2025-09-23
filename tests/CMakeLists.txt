project(tests)
add_executable(${PROJECT_NAME} main.cpp orderbook_tests.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
