cmake_minimum_required(VERSION 3.15)
project(triad LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(triadc
  src/triadc.cpp
  src/triad_parser.cpp
  src/triad_lexer.hpp
  src/triad_ast.hpp
  src/triad_bytecode.hpp
  src/triad_vm.cpp
)
target_include_directories(triadc PRIVATE src)
