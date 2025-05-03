cmake_minimum_required(VERSION 3.16)
project(TATLIN-UNIFIED-test-task)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR/tests})

add_executable(tape_sorter src/main.cpp)
target_include_directories(tape_sorter PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(extern/googletest)
add_executable(tape_tests tests/tape_tests.cpp)
target_include_directories(tape_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tape_tests gtest gtest_main)
