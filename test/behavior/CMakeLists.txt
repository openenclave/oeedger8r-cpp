# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

function (add_behavior_test test_name edl pass_re fail_re)
  add_test(NAME ${test_name} COMMAND oeedger8r --header-only --search-path
                                     ${CMAKE_CURRENT_SOURCE_DIR} ${edl})
  if (${fail_re})
    set_tests_properties(
      ${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}"
                              FAIL_REGULAR_EXPRESSION "${fail_re}")
  else ()
    set_tests_properties(${test_name} PROPERTIES PASS_REGULAR_EXPRESSION
                                                 "${pass_re}")
  endif ()
endfunction ()

# `private` specified should generate warning.
add_behavior_test(
  oeedger8r_private_trusted_warning private_trusted.edl
  "error: Function 'private': 'private' specifier is not supported by oeedger8r"
  "")

# `allow` list should generate warning.
add_behavior_test(
  oeedger8r_allow_list_warning
  allow_list.edl
  "Warning: Function 'ocall_allow': Reentrant ocalls are not supported by Open Enclave. Allow list ignored."
  "")

# These need to be separate tests to ensure that each type, for both
# trusted and untrusted functions, generate the appropriate warning,
# but we can reuse the EDL file.
add_behavior_test(oeedger8r_portability_trusted_wchar_t_warning portability.edl
                  "Function 'ocall': wchar_t has different sizes" "")

add_behavior_test(
  oeedger8r_portability_trusted_long_double_warning portability.edl
  "Function 'ocall': long double has different sizes" "")

add_behavior_test(oeedger8r_portability_trusted_long_warning portability.edl
                  "Function 'ocall': long has different sizes" "")

add_behavior_test(
  oeedger8r_portability_trusted_unsigned_long_warning portability.edl
  "Function 'ocall': unsigned long has different sizes" "")

add_behavior_test(
  oeedger8r_portability_untrusted_wchar_t_warning portability.edl
  "Function 'ecall': wchar_t has different sizes" "")

add_behavior_test(
  oeedger8r_portability_untrusted_long_double_warning portability.edl
  "Function 'ecall': long double has different sizes" "")

add_behavior_test(oeedger8r_portability_untrusted_long_warning portability.edl
                  "Function 'ecall': long has different sizes" "")

add_behavior_test(
  oeedger8r_portability_untrusted_unsigned_long_double_warning portability.edl
  "Function 'ecall': unsigned long has different sizes" "")

# Test signedness of size and count
add_behavior_test(
  oeedger8r_size_signedness_warning
  size_signedness.edl
  "Warning: Function 'signed_size': Size or count parameter 'size' should not be signed."
  "Warning: Function 'unsigned_size': Size or count parameter 'size' should not be signed."
)

add_behavior_test(
  oeedger8r_count_signedness_warning
  count_signedness.edl
  "Warning: Function 'signed_count': Size or count parameter 'count' should not be signed."
  "Warning: Function 'unsigned_count': Size or count parameter 'count' should not be signed."
)

add_behavior_test(
  oeedger8r_size_and_count_warning
  size_and_count.edl
  "Function 'size_and_count': simultaneous 'size' and 'count' parameters 'size' and 'count' are not supported by oeedger8r."
  "")

add_behavior_test(
  oeedger8r_deepcopy_value_warning
  deepcopy_value.edl
  "error: the structure declaration \"MyStruct\" specifies a deep copy is expected. Referenced by value in function \"deepcopy_value\" detected."
  "")

