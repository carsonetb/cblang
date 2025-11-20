cmake_minimum_required(VERSION 3.15)
project(cblang_test CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(cblang CONFIG REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example cblang::cblang)