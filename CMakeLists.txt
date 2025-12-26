cmake_minimum_required(VERSION 3.15)
project(cblang CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0 -frtti -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

find_package(spdlog REQUIRED)



add_library(cblang 
    src/cblang.cpp 
    src/compiler.cpp 
    src/definitions.cpp 
    src/objects.cpp 
    src/parser.cpp 
    src/program.cpp
    src/scanner.cpp 
    src/standard.cpp
    src/util.cpp
)
target_include_directories(cblang PUBLIC include)
target_link_libraries(cblang PRIVATE spdlog::spdlog)

set_target_properties(cblang PROPERTIES PUBLIC_HEADER "include/cblang.h;include/definitions.h;include/objects.h;include/compiler.h;include/scanner.h;include/parser.h;include/program.h;include/standard.hpp")
install(TARGETS cblang)
