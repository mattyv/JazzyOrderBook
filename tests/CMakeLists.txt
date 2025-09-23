project(tests)
add_executable(${PROJECT_NAME} main.cpp orderbook_tests.cpp select_nth_test.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE jazzy)
# Add GCC-specific compile options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -fconcepts-diagnostics-depth=10)
endif()
