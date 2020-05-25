# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT basic_args.h basic_t.h basic_t.c
  COMMAND oeedger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../basic.edl)

add_library(oeedger8r_basic_enc SHARED basic_t.c enc.cpp)

target_include_directories(oeedger8r_basic_enc
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_basic_enc oeedger8r_test_enclave)

set_target_properties(oeedger8r_basic_enc PROPERTIES PREFIX "")
