# Copyright (c) Open Enclave SDK contributors. Licensed under the MIT License.

function(add_import_test test_name edl pass_re fail_re)
  add_test(NAME ${test_name} COMMAND oeedger8r --header-only --search-path
                                     ${CMAKE_CURRENT_SOURCE_DIR} ${edl})
  if(${fail_re})
    set_tests_properties(
      ${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}"
                              FAIL_REGULAR_EXPRESSION "${fail_re}")
  else()
    set_tests_properties(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION
                                                 "${pass_re}")
  endif()
endfunction()

# Test selective import of items from a.edl.
add_import_test(oeedger8r_selective_import1 b0.edl "Success." "")

# Test multiple import of a.edl
add_import_test(oeedger8r_multiple_import1 b1.edl "Success." "")

# Defining an ecall with same name as imported ecall should fail.
add_import_test(
  oeedger8r_import_ecall_redefinition_error1 b2.edl
  "Duplicate function definition detected for a_ecall1" "Success.")

# Defining an ecall with same name as imported ocall should fail.
add_import_test(
  oeedger8r_import_ecall_redefinition_error2 b3.edl
  "Duplicate function definition detected for a_ecall1" "Success.")

# Defining an ocall with same name as imported ocall should fail.
add_import_test(
  oeedger8r_import_ocall_redefinition_error1 b4.edl
  "Duplicate function definition detected for a_ocall1" "Success.")

# Defining an ocall with same name as imported ecall should fail.
add_import_test(
  oeedger8r_import_ocall_redefinition_error2 b5.edl
  "Duplicate function definition detected for a_ocall1" "Success.")

# Defining an enum with same name as imported type should fail.
add_import_test(oeedger8r_import_type_redefinition_error1 b6.edl
                "Duplicate type definition detected for a_struct1" "Success.")

# Defining an struct with same name as imported type should fail.
add_import_test(oeedger8r_import_type_redefinition_error2_error b7.edl
                "Duplicate type definition detected for a_struct1" "Success.")

# Importing a file with duplicate type definitions should error.
add_import_test(oeedger8r_import_duplicate_type_definitions_error b8.edl
                "Duplicate type definition detected for a_struct1" "Success.")

# Importing a file with duplicate ecalls should error.
add_import_test(
  oeedger8r_import_duplicate_ecall_definitions_error b9.edl
  "Duplicate function definition detected for a_ecall1" "Success.")

# Importing a file with duplicate ocalls should error.
add_import_test(
  oeedger8r_import_duplicate_ocall_definitions_error b10.edl
  "Duplicate function definition detected for a_ocall1" "Success.")

# Importing a file with same name ocall and ecall should error.
add_import_test(oeedger8r_import_same_name_ocall_ecall_error b11.edl
                "Duplicate function definition detected for foo" "Success.")

# Importing same function via a diamong import should succeed.
add_import_test(oeedger8r_import_diamond diamond_d.edl "Success." "Duplicate")

# Importing an edl with same basename is allowed.
add_import_test(oeedger8r_import_same_basename samename.edl "Success."
                "Recursive")

# Ensure that a.edl is picked up from first search-path. Warning will be raised
# for function import_dir1_ecall.
add_test(
  NAME oeedger8r_import_search_path_order1
  COMMAND oeedger8r a.edl --search-path ${CMAKE_CURRENT_SOURCE_DIR}/import_dir1
          --search-path ${CMAKE_CURRENT_SOURCE_DIR}/import_dir2)
set_tests_properties(
  oeedger8r_import_search_path_order1
  PROPERTIES PASS_REGULAR_EXPRESSION "Warning: Function 'import_dir1_ecall'")

# Ensure that a.edl is picked up from first search-path. Warning will be raised
# for function import_dir2_ecall.
add_test(
  NAME oeedger8r_import_search_path_order2
  COMMAND oeedger8r a.edl --search-path ${CMAKE_CURRENT_SOURCE_DIR}/import_dir2
          --search-path ${CMAKE_CURRENT_SOURCE_DIR}/import_dir1)
set_tests_properties(
  oeedger8r_import_search_path_order2
  PROPERTIES PASS_REGULAR_EXPRESSION "Warning: Function 'import_dir2_ecall'")

# Lockdown printing of imported file being processed.
add_test(
  NAME oeedger8r_import_print_processing
  COMMAND oeedger8r processing.edl --search-path ${CMAKE_CURRENT_SOURCE_DIR}
          --search-path ${CMAKE_CURRENT_SOURCE_DIR}/import_dir3/x)

file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/import_dir3/x/y/z.edl"
     IMPORTED_FILE_PATH)
# \ character becomes \\\\ in a regular expression.
# Convert \ to \\ in IMPORTED_FILE_PATH.
string(REGEX REPLACE "\\\\" "\\\\\\\\" IMPORTED_FILE_PATH ${IMPORTED_FILE_PATH})

set_tests_properties(
  oeedger8r_import_print_processing
  PROPERTIES PASS_REGULAR_EXPRESSION "Processing ${IMPORTED_FILE_PATH}.")
