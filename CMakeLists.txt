# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.0)

project(oeedger8r)

add_executable(oeedger8r main.cpp parser.cpp lexer.cpp)
set_property(TARGET oeedger8r PROPERTY POSITION_INDEPENDENT_CODE on)

if (UNIX AND NOT USE_OEEDGER8R_CPP_AS_SUBMODULE)
  # corelibc causes problems when compiling the enclave in virtual mode
  # using msvc. Enable tests for now only on Linux.
  # Also don't run tests when used as a sumbodule since OE SDK will
  # thoroughly test oeedger8r on both Linux and Windows.
  enable_testing()
  add_subdirectory(test)
endif ()
