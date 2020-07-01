# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT preprocessor_args.h preprocessor_t.h preprocessor_t.c
  COMMAND
    oeedger8r --trusted -DTEST_ECALL -DTEST_ENUM -DTEST_OCALL -DTEST_STRUCT
    -DTEST_NESTED_IFDEF_ECALL -DTEST_IMPORT -DTEST_IMPORT_FROM_ALL
    -DTEST_IMPORT_FROM --search-path ${CMAKE_CURRENT_SOURCE_DIR}/../edl
    # Definitions for complex preprocessor usage
    -DCOMPLEX1 -DCOMPLEX2 -DCOMPLEX3 -DCOMPLEX4 preprocessor.edl)

add_library(oeedger8r_preprocessor_enc SHARED preprocessor_t.c enc.cpp)

target_include_directories(oeedger8r_preprocessor_enc
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_preprocessor_enc oeedger8r_test_enclave)

set_target_properties(oeedger8r_preprocessor_enc PROPERTIES PREFIX "")
