# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

# Version 3.1 required for CMAKE_CXX_STANDARD
cmake_minimum_required(VERSION 3.1)

# Set CMAKE_BUILD_TYPE if not specified
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif ()

if (NOT ";Debug;RelWithDebInfo;Release;" MATCHES ";${CMAKE_BUILD_TYPE};")
  message(
    FATAL_ERROR "CMAKE_BUILD_TYPE must be Debug, RelWithDebInfo or Release")
endif ()

project(oeedger8r)

# Collect Git info
if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/pre-commit scripts/commit-msg
       DESTINATION "${PROJECT_SOURCE_DIR}/.git/hooks")
endif ()

set(CMAKE_CXX_STANDARD 11)
add_executable(oeedger8r main.cpp parser.cpp lexer.cpp)
set_property(TARGET oeedger8r PROPERTY POSITION_INDEPENDENT_CODE on)

enable_testing()
add_subdirectory(test)

option(CODE_COVERAGE "Enable code coverage testing" OFF)
if (CODE_COVERAGE)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(FATAL_ERROR "Code coverage is supported only with GCC.")
  endif ()

  message("Building for code coverage.")
  target_compile_options(oeedger8r PRIVATE -g -O0 -coverage)
  target_link_libraries(oeedger8r PRIVATE -coverage)

  add_custom_target(
    code_coverage
    # Build only edger8r target.
    # Other targets will be (re)built by code-coverage script.
    COMMAND cmake --build . -j 4 --target oeedger8r
    COMMAND ${PROJECT_SOURCE_DIR}/scripts/code-coverage .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()
