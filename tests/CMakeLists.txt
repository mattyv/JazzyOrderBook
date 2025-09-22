project(tests)
add_executable(${PROJECT_NAME} main.cpp orderbook_tests.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy)
