cmake_minimum_required(VERSION 3.15)
project(cblang CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



add_library(cblang src/cblang.cpp)
target_include_directories(cblang PUBLIC include)



set_target_properties(cblang PROPERTIES PUBLIC_HEADER "include/cblang.h")
install(TARGETS cblang)
