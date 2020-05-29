# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

function (add_cmdline_test test_name cmdline pass_re fail_re)
  separate_arguments(cmdline)
  add_test(NAME ${test_name} COMMAND oeedger8r ${cmdline})
  if (${fail_re})
    set_tests_properties(
      ${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}"
                              FAIL_REGULAR_EXPRESSION "${fail_re}")
  else ()
    set_tests_properties(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION
                                                 "${pass_re}")
  endif ()
endfunction ()

add_cmdline_test(
  oeedger8r_missing_trusted_dir
  "${CMAKE_CURRENT_SOURCE_DIR}/../basic/basic.edl --trusted-dir"
  "error: missing directory name after --trusted-dir" "")

add_cmdline_test(
  oeedger8r_missing_untrusted_dir
  "${CMAKE_CURRENT_SOURCE_DIR}/../basic/basic.edl --untrusted-dir"
  "error: missing directory name after --untrusted-dir" "")

add_cmdline_test(
  oeedger8r_missing_search_path
  "${CMAKE_CURRENT_SOURCE_DIR}/../basic/basic.edl --search-path"
  "error: missing directory name after --search-path" "")

# Check whether --trusted-dir and --untrusted-dir works.
set(TRUSTED_DIR "${CMAKE_CURRENT_BINARY_DIR}/trusted-dir/foo/../bar/x")
set(UNTRUSTED_DIR "${CMAKE_CURRENT_BINARY_DIR}/untrusted-dir/foo/../bar/y")
set(EDL ${CMAKE_CURRENT_SOURCE_DIR}/../basic/basic.edl)
add_custom_command(
  OUTPUT ${TRUSTED_DIR}/basic_t.c ${UNTRUSTED_DIR}/basic_u.c
  COMMAND oeedger8r --trusted-dir ${TRUSTED_DIR} ${EDL} --untrusted-dir
          ${UNTRUSTED_DIR}
  DEPENDS ${EDL})

add_library(oeedger8r_test_dirs STATIC ${TRUSTED_DIR}/basic_t.c
                                       ${UNTRUSTED_DIR}/basic_u.c)

target_link_libraries(oeedger8r_test_dirs oeedger8r_test_host)
