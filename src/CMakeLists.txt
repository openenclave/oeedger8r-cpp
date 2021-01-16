# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_executable(oeedger8r main.cpp parser.cpp lexer.cpp)
set_property(TARGET oeedger8r PROPERTY POSITION_INDEPENDENT_CODE on)

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
