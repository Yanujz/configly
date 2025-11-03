# cmake_minimum_required(VERSION 3.20)
# project(example CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)


# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../ ${CMAKE_BINARY_DIR}/configly)


# add_executable(${PROJECT_NAME} main.cpp)
# target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../ )




cmake_minimum_required(VERSION 3.14)
project(BasicUsageExample)

# Aggiunge l'eseguibile
add_executable(basic_usage main.cpp)

# Collega la libreria configly al nostro esempio
target_link_libraries(basic_usage PRIVATE configly)