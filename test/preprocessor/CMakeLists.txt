# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_custom_command(
  OUTPUT preprocessor_args.h preprocessor_t.h preprocessor_t.c
  COMMAND oeedger8r -DTEST --trusted ${CMAKE_CURRENT_SOURCE_DIR}/preprocessor.edl)

add_custom_target(
  build_preprocessor_test ALL
  DEPENDS preprocessor_args.h preprocessor_t.h preprocessor_t.c)

function (add_preprocessor_test test_name cmdline actual_file expected_file)
  separate_arguments(cmdline)
  add_custom_command(
    OUTPUT __dummy
    COMMAND oeedger8r ${cmdline} ALL)

  add_test(
    NAME ${test_name}
    COMMAND ${CMAKE_COMMAND} -E
            compare_files --ignore-eol ${actual_file} ${expected_file})
endfunction ()

add_preprocessor_test(
  oeedger8r_basic_preprocessor
  "${CMAKE_CURRENT_SOURCE_DIR}/preprocessor.edl -DTEST --trusted-dir"
  "${CMAKE_CURRENT_BINARY_DIR}/preprocessor.edl.out"
  "${CMAKE_CURRENT_SOURCE_DIR}/preprocessor.edl.expected")

