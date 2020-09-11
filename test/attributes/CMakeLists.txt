# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

function (add_attributes_test test_name pass_re fail_re)
  add_test(NAME oeedger8r_${test_name}
           COMMAND oeedger8r --header-only --search-path
                   ${CMAKE_CURRENT_SOURCE_DIR} -D${test_name} attributes.edl)
  if (${fail_re})
    set_tests_properties(
      oeedger8r_${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${PASS_RE}"
                                        FAIL_REGULAR_EXPRESSION "${fail_re}")
  else ()
    set_tests_properties(oeedger8r_${test_name}
                         PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}")
  endif ()
endfunction ()

# Checks for string/wstring
add_attributes_test(
  OUT_STRING "string/wstring attribute should be used with an `in' attribute"
  "")

add_attributes_test(
  OUT_WSTRING "string/wstring attribute should be used with an `in' attribute"
  "")

add_attributes_test(
  STRING_NO_DIR "string/wstring attributes must be used with pointer direction"
  "")

add_attributes_test(
  WSTRING_NO_DIR
  "string/wstring attributes must be used with pointer direction" "")

add_attributes_test(
  STRING_COUNT
  "size attributes are mutually exclusive with \\(w\\)string attribute" "")

add_attributes_test(
  WSTRING_COUNT
  "size attributes are mutually exclusive with \\(w\\)string attribute" "")

add_attributes_test(
  STRING_SIZE
  "size attributes are mutually exclusive with \\(w\\)string attribute" "")

add_attributes_test(
  WSTRING_SIZE
  "size attributes are mutually exclusive with \\(w\\)string attribute" "")

add_attributes_test(STRING_TYPE1 "unexpected pointer attributes for" "")

add_attributes_test(STRING_TYPE2
                    "invalid `string' attribute - `str' is not char pointer" "")

add_attributes_test(WSTRING_TYPE1 "unexpected pointer attributes for" "")

add_attributes_test(
  WSTRING_TYPE2 "invalid `wstring' attribute - `str' is not wchar_t pointer" "")

# Checks for isary/isptr
add_attributes_test(ISARY_ISPTR "`isary' cannot be used with `isptr' together"
                    "")

add_attributes_test(
  ISARY_NO_DIR "`isary' should have direction attribute or `user_check'" "")

add_attributes_test(
  ISPTR_NO_DIR "`isptr' should have direction attribute or `user_check'" "")

add_attributes_test(
  ISARY_TYPE1
  "`isary' attribute is only valid for user defined type, not for `int'" "")

add_attributes_test(
  ISARY_TYPE2
  "`isary' attribute is only valid for user defined type, not for `int\\*'" "")

add_attributes_test(
  ISPTR_TYPE1
  "`isptr' attribute is only valid for user defined type, not for `int'" "")

add_attributes_test(
  ISPTR_TYPE2
  "`isptr' attribute is only valid for user defined type, not for `int\\*'" "")

# Checks for user_check
add_attributes_test(
  USER_CHECK_IN "pointer direction and `user_check' are mutually exclusive" "")

add_attributes_test(
  USER_CHECK_OUT "pointer direction and `user_check' are mutually exclusive" "")

add_attributes_test(
  USER_CHECK_IN_OUT "pointer direction and `user_check' are mutually exclusive"
  "")

add_attributes_test(
  USER_CHECK_SIZE "size/count attributes must be used with pointer direction"
  "")

add_attributes_test(
  USER_CHECK_COUNT "size/count attributes must be used with pointer direction"
  "")

add_attributes_test(
  USER_CHECK_STRING
  "string/wstring attributes must be used with pointer direction" "")

add_attributes_test(
  USER_CHECK_WSTRING
  "string/wstring attributes must be used with pointer direction" "")

add_attributes_test(
  USER_CHECK_PLAIN_TYPE
  "`user_check' attribute is invalid for plain type `mytype'" "")

# Checks for in/out
add_attributes_test(IN_TYPE1 "unexpected pointer attributes for `int'" "")

add_attributes_test(IN_TYPE2 "`in' is invalid for plain type `mytype'" "")

add_attributes_test(OUT_TYPE1 "unexpected pointer attributes for `int'" "")

add_attributes_test(OUT_TYPE2 "`out' is invalid for plain type `mytype'" "")

# Checks for count/size
add_attributes_test(
  COUNT_NO_DIR "size/count attributes must be used with pointer direction" "")

add_attributes_test(
  SIZE_NO_DIR "size/count attributes must be used with pointer direction" "")

add_attributes_test(COUNT_TYPE "unexpected pointer attributes for `int'" "")

add_attributes_test(COUNT_PLAIN_TYPE
                    "`count' is invalid for plain type `mytype'" "")

add_attributes_test(SIZE_PLAIN_TYPE "`size' is invalid for plain type `mytype'"
                    "")

add_attributes_test(COUNT_INVALID_PROP1
                    "could not find declaration for 'count_prop'" "")

add_attributes_test(COUNT_INVALID_PROP2 "size/count has invalid type" "")

add_attributes_test(SIZE_INVALID_PROP1
                    "could not find declaration for 'size_prop'" "")

add_attributes_test(SIZE_INVALID_PROP2 "size/count has invalid type" "")
