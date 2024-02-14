cmake_minimum_required(VERSION 3.10)

project(database CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDebug -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

file(GLOB_RECURSE SRC "src/*.cpp")

include_directories(src)
include_directories(include/antlr4-runtime)

add_subdirectory(include/antlr4-runtime)

add_executable(database ${SRC})
target_link_libraries(database antlr4_runtime)