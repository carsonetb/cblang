cmake_minimum_required(VERSION 3.15)
project(cblang CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)



add_library(cblang src/cblang.cpp src/definitions.cpp src/lexical.cpp src/objects.cpp src/compiler.cpp)
target_include_directories(cblang PUBLIC include)
target_link_libraries(cblang PRIVATE spdlog::spdlog)



set_target_properties(cblang PROPERTIES PUBLIC_HEADER "include/cblang.h;include/definitions.h;include/lexical.h;include/objects.h;include/compiler.h")
install(TARGETS cblang)
