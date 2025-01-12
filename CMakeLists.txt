cmake_minimum_required(VERSION 3.20)

project(configly VERSION 1.0 DESCRIPTION "Flexible configuration management with real-time updates and callbacks.")

add_library(configly INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# enable_testing()
# add_subdirectory(tests)

