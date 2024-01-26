cmake_minimum_required(VERSION 3.20)
project(DarkMxCompiler)

set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(/usr/include/antlr4-runtime/)
include_directories(include)
include_directories(include/AST)
include_directories(include/IR)
include_directories(src/generated)


file(GLOB_RECURSE sources src/*.cpp)
add_executable(mxc ${sources})
target_link_libraries(mxc PUBLIC antlr4-runtime)
