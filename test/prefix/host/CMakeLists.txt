# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT enc1_args.h enc1_u.h enc1_u.c
  COMMAND oeedger8r --untrusted --search-path ${CMAKE_CURRENT_SOURCE_DIR}/..
          enc1.edl
  DEPENDS ../common.edl ../enc1.edl)

add_custom_command(
  OUTPUT enc2_args.h enc2_u.h enc2_u.c
  COMMAND oeedger8r --untrusted --search-path ${CMAKE_CURRENT_SOURCE_DIR}/..
          enc2.edl --use-prefix
  DEPENDS ../common.edl ../enc2.edl)

add_executable(oeedger8r_prefix_host enc1_u.c enc2_u.c host.cpp)

target_include_directories(oeedger8r_prefix_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_prefix_host oeedger8r_test_host)
