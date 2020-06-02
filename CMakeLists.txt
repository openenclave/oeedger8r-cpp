# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

# Version 3.1 required for CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

project(oeedger8r)

set(CMAKE_CXX_STANDARD 11)
add_executable(oeedger8r main.cpp parser.cpp lexer.cpp preprocessor.cpp)
set_property(TARGET oeedger8r PROPERTY POSITION_INDEPENDENT_CODE on)

enable_testing()
add_subdirectory(test)

