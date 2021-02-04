# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT safe_math_args.h safe_math_u.h safe_math_u.c
  COMMAND oeedger8r --experimental --untrusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../safe_math.edl)

add_executable(oeedger8r_safe_math_host safe_math_u.c host.cpp)

target_include_directories(oeedger8r_safe_math_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_safe_math_host oeedger8r_test_host)
