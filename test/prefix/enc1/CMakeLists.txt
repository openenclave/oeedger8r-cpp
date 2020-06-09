# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT enc1_args.h enc1_t.h enc1_t.c
  COMMAND oeedger8r --trusted --search-path ${CMAKE_CURRENT_SOURCE_DIR}/..
          enc1.edl
  DEPENDS ../common.edl ../enc1.edl)

add_library(oeedger8r_enc1 SHARED enc1_t.c enc.c)

target_include_directories(oeedger8r_enc1 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_enc1 oeedger8r_test_enclave)

set_target_properties(oeedger8r_enc1 PROPERTIES PREFIX "")
