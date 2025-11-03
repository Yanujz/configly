cmake_minimum_required(VERSION 3.14)
project(ConfiglyTests)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(configly_tests main_test.cpp)

target_link_libraries(configly_tests PRIVATE configly gtest_main)

include(GoogleTest)
gtest_add_tests(TARGET configly_tests)