# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT enc2_args.h enc2_t.h enc2_t.c
  COMMAND oeedger8r --trusted --search-path ${CMAKE_CURRENT_SOURCE_DIR}/..
          enc2.edl
  DEPENDS ../common.edl ../enc2.edl)

add_library(oeedger8r_enc2 SHARED enc2_t.c enc.c)

target_include_directories(oeedger8r_enc2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_enc2 oeedger8r_test_enclave)

set_target_properties(oeedger8r_enc2 PROPERTIES PREFIX "")
