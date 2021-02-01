# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT safe_math_args.h safe_math_t.h safe_math_t.c
  COMMAND oeedger8r --experimental --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../safe_math.edl)

add_library(oeedger8r_safe_math_enc SHARED safe_math_t.c enc.cpp)

target_include_directories(oeedger8r_safe_math_enc
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_safe_math_enc oeedger8r_test_enclave)

set_target_properties(oeedger8r_safe_math_enc PROPERTIES PREFIX "")
