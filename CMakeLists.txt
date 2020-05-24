# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.0)
project(oeedger8r)

add_executable(oeedger8r main.cpp parser.cpp lexer.cpp)
set_property(TARGET oeedger8r PROPERTY POSITION_INDEPENDENT_CODE on)

add_subdirectory(test)
