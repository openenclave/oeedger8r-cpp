# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT basic_args.h basic_u.h basic_u.c
  COMMAND oeedger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../basic.edl)

add_executable(oeedger8r_basic_host basic_u.c host.cpp)

target_include_directories(oeedger8r_basic_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_basic_host oeedger8r_test_host)
