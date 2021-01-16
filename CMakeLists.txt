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

# Set compiler search order to prefer Clang
# This has to be done before `project`
# http://cmake.3232098.n2.nabble.com/Prefer-clang-over-gcc-td7597742.html
if (UNIX)
  set(CMAKE_C_COMPILER_NAMES clang-8 clang-7 clang cc)
  set(CMAKE_CXX_COMPILER_NAMES clang++-8 clang++-7 clang c++)
else ()
  # Prefer platform's default compiler.
endif ()

project(oeedger8r)

# Collect Git info
if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/pre-commit scripts/commit-msg
       DESTINATION "${PROJECT_SOURCE_DIR}/.git/hooks")
endif ()

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

option(CODE_COVERAGE "Enable code coverage testing" OFF)
