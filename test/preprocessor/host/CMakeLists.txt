# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

add_custom_command(
  OUTPUT preprocessor_args.h preprocessor_u.h preprocessor_u.c
  COMMAND
    oeedger8r --untrusted -DTEST_ECALL -DTEST_ENUM -DTEST_OCALL -DTEST_STRUCT
    -DTEST_NESTED_IFDEF_ECALL -DTEST_IMPORT -DTEST_IMPORT_FROM_ALL
    -DTEST_IMPORT_FROM --search-path ${CMAKE_CURRENT_SOURCE_DIR}/../edl
    # Definitions for complex preprocessor usage
    -DCOMPLEX1 -DCOMPLEX2 -DCOMPLEX3 -DCOMPLEX4 preprocessor.edl)

add_executable(oeedger8r_preprocessor_host preprocessor_u.c host.cpp)

target_include_directories(oeedger8r_preprocessor_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeedger8r_preprocessor_host oeedger8r_test_host)
