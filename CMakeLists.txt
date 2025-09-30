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

# If CC environment variable has been specified or if CMAKE_C_COMPILER
# cmake variable has been passed to cmake, use the C compiler that has
# been specified. Otherwise, prefer clang. Same for C++ compiler.
# This must be done before `project` command.
if (UNIX)
  if (NOT DEFINED ENV{CC} AND NOT DEFINED CMAKE_C_COMPILER)
    find_program(CMAKE_C_COMPILER clang-10 clang)
  endif ()
  if (NOT DEFINED ENV{CXX} AND NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CMAKE_CXX_COMPILER clang++-10 clang++)
  endif ()
endif ()

project(oeedger8r)

# Collect Git info
if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/pre-commit scripts/commit-msg
       DESTINATION "${PROJECT_SOURCE_DIR}/.git/hooks")
endif ()

option(BUILD_TESTS "Build oeedger8r virtual environment tests" ON)

set(CMAKE_CXX_STANDARD 17)

# Set Windows-specific compiler flags
if (WIN32 AND MSVC)
  # Enable warning level 3 and treat warnings as errors
  add_compile_options(/W3 /WX)

  # Enable specific warnings as errors
  add_compile_options(
    /we4018 # signed/unsigned mismatch
    /we4055 # type cast from data pointer to function pointer
    /we4146 # unary minus operator applied to unsigned type
    /we4242 # conversion from 'type1' to 'type2', possible loss of data
    /we4244 # conversion from 'type1' to 'type2', possible loss of data
    /we4267 # conversion from 'size_t' to 'type', possible loss of data
    /we4302 # truncation from 'type1' to 'type2'
    /we4308 # negative integral constant converted to unsigned type
    /we4509 # nonstandard extension used: function uses SEH and object has destructor
    /we4510 # default constructor could not be generated
    /we4532 # jump out of __finally block has undefined behavior
    /we4533 # initialization of variable is skipped by jump instruction
    /we4610 # object can never be instantiated - user-defined constructor required
    /we4611 # interaction between setjmp and C++ object destruction is non-portable
    /we4700 # uninitialized local variable used
    /we4701 # potentially uninitialized local variable used
    /we4703 # potentially uninitialized local pointer variable used
    /we4789 # buffer overrun when using secure functions
    /we4995 # function marked as #pragma deprecated
    /we4996 # function declared deprecated
  )
endif ()

add_subdirectory(src)

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif ()

option(CODE_COVERAGE "Enable code coverage testing" OFF)
