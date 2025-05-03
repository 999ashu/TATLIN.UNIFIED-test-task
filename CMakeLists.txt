cmake_minimum_required(VERSION 3.16)
project(TATLIN-UNIFIED-test-task)

set(CMAKE_CXX_STANDARD 20)

add_executable(tape_sorter src/tape_sorter.cpp)
target_include_directories(tape_sorter PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(extern/googletest)
add_executable(tests tests/tests.cpp)
target_link_libraries(tests gtest gtest_main)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
