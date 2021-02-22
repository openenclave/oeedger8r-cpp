# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

function (add_warnings_test test_name options pass_re fail_re)
  add_test(
    NAME oeedger8r_${test_name}
    COMMAND oeedger8r --header-only --search-path ${CMAKE_CURRENT_SOURCE_DIR}
            -D${test_name} ${options} warnings.edl)
  if (fail_re)
    set_tests_properties(
      oeedger8r_${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}"
                                        FAIL_REGULAR_EXPRESSION "${fail_re}")
  else ()
    set_tests_properties(oeedger8r_${test_name}
                         PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}")
  endif ()
endfunction ()

function (add_werror_test test_name options pass_re fail_re)
  add_test(NAME oeedger8r_${test_name}
           COMMAND oeedger8r --header-only --search-path
                   ${CMAKE_CURRENT_SOURCE_DIR} ${options} werror.edl)
  if (fail_re)
    set_tests_properties(
      oeedger8r_${test_name} PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}"
                                        FAIL_REGULAR_EXPRESSION "${fail_re}")
  else ()
    set_tests_properties(oeedger8r_${test_name}
                         PROPERTIES PASS_REGULAR_EXPRESSION "${pass_re}")
  endif ()
endfunction ()

# Warning tests

add_warnings_test(
  PTR_IN_STRUCT
  "-Wptr-in-struct"
  "warning: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
  "")

add_warnings_test(
  NO_PTR_IN_STRUCT
  "-Wptr-in-struct"
  ""
  "warning: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
)

add_warnings_test(
  WALL_PTR_IN_STRUCT
  "-Wall"
  "warning: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
  "")

add_warnings_test(
  WALL_NO_PTR_IN_STRUCT
  "-Wall;-Wno-ptr-in-struct"
  ""
  "warning: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
)

add_warnings_test(
  RETURN_PTR_TYPE1
  "-Wreturn-ptr"
  "warning: .* Function `func': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_warnings_test(
  RETURN_PTR_TYPE2
  "-Wreturn-ptr"
  "warning: .* Function `func': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_warnings_test(
  RETURN_PTR_TYPE3
  "-Wreturn-ptr"
  "warning: .* Function `func': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_warnings_test(
  WALL_RETURN_PTR
  "-Wall"
  "warning: .* Function `func': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_warnings_test(
  WALL_NO_RETURN_PTR
  "-Wall;-Wno-return-ptr"
  ""
  "warning: .* Function `func': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
)

add_warnings_test(
  PTR_IN_FUNCTION_TYPE1
  "-Wptr-in-function"
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_warnings_test(
  PTR_IN_FUNCTION_TYPE2
  "-Wptr-in-function"
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_warnings_test(
  PTR_IN_FUNCTION_TYPE3
  "-Wptr-in-function"
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_warnings_test(
  PTR_IN_FUNCTION_TYPE4
  "-Wptr-in-function"
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_warnings_test(
  WALL_PTR_IN_FUNCTION
  "-Wall"
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_warnings_test(
  WALL_NO_PTR_IN_FUNCTION
  "-Wall;-Wno-ptr-in-function"
  ""
  "warning: .* Function `func': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
)

add_warnings_test(
  FOREIGN_TYPE_PTR_TYPE1
  "-Wforeign-type-ptr"
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_warnings_test(
  FOREIGN_TYPE_PTR_TYPE2
  "-Wforeign-type-ptr"
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_warnings_test(
  FOREIGN_TYPE_PTR_TYPE3
  "-Wforeign-type-ptr"
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_warnings_test(
  WALL_FOREIGN_TYPE_PTR
  "-Wall"
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_warnings_test(
  WALL_NO_FOREIGN_TYPE_PTR
  "-Wall;-Wno-foreign-type-ptr"
  ""
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_warnings_test(
  NON_FOREIGN_TYPE_PTR_TYPE1
  "-Wforeign-type-ptr"
  ""
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_warnings_test(
  NON_FOREIGN_TYPE_PTR_TYPE2
  "-Wforeign-type-ptr"
  ""
  "warning: .* Function `func': `s' is a pointer of a foreign type `MyStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_warnings_test(
  NON_FOREIGN_TYPE_PTR_TYPE3
  "-Wforeign-type-ptr"
  ""
  "warning: .* Function `func': `s' is a pointer of a foreign type .* that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_warnings_test(
  NON_FOREIGN_TYPE_PTR_TYPE4
  "-Wforeign-type-ptr"
  ""
  "warning: .* Function `func': `s' is a pointer of a foreign type .* that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_warnings_test(
  UNSUPPORTED_ALLOW_TYPE1
  "-Wunsupported-allow"
  "warning: .* Function `func': the `allow' syntax is currently unsupported. Ignored .-Wunsupported-allow."
  "")

add_warnings_test(
  UNSUPPORTED_ALLOW_TYPE2
  "-Wunsupported-allow"
  "warning: .* Function `func': the `allow' syntax is currently unsupported. Ignored .-Wunsupported-allow."
  "")

# Werror tests

add_werror_test(
  WERROR_RETURN_PTR
  "-Werror;-Wreturn-ptr"
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_werror_test(
  WERROR_EQUAL_TO_RETURN_PTR_TYPE1
  "-Werror=return-ptr"
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_werror_test(
  WERROR_EQUAL_TO_RETURN_PTR_TYPE2
  "-Wreturn-ptr;-Werror=return-ptr"
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "")

add_werror_test(
  WERROR_NO_RETURN_PTR_TYPE1
  "-Werror;-Wreturn-ptr;-Wno-return-ptr"
  ""
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
)

add_werror_test(
  WERROR_NO_RETURN_PTR_TYPE2
  "-Werror=return-ptr;-Wno-return-ptr"
  ""
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
)

add_werror_test(
  WERROR_NO_RETURN_PTR_TYPE3
  "-Wreturn-ptr;-Werror=return-ptr;-Wno-return-ptr"
  ""
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
)

add_werror_test(
  NO_WERROR_RETURN_PTR
  "-Werror;-Wreturn-ptr;-Wno-error"
  "warning: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
  "error: .* Function `foo': The function returns a pointer, which could expose memory addresses across the host-enclave boundary. Consider passing the pointer as an out parameter instead .-Wreturn-ptr."
)

add_werror_test(
  WERROR_PTR_IN_STRUCT
  "-Werror;-Wptr-in-struct"
  "error: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
  "")

add_werror_test(
  WERROR_EQUAL_TO_PTR_IN_STRUCT
  "-Werror=ptr-in-struct"
  "error: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
  "")

add_werror_test(
  WERROR_NO_PTR_IN_STRUCT
  "-Werror;-Wptr-in-struct;-Wno-ptr-in-struct"
  ""
  "error: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
)

add_werror_test(
  NO_WERROR_PTR_IN_STRUCT
  "-Werror;-Wptr-in-struct;-Wno-error"
  "warning: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
  "error: .* struct `MyStruct': The member `p' is a pointer that is not serializable. Consider annotating the member with the `count' or `size' attribute .-Wptr-in-struct."
)

add_werror_test(
  WERROR_PTR_IN_FUNCTION
  "-Werror;-Wptr-in-function"
  "error: .* Function `bar': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_werror_test(
  WERROR_EQUAL_TO_PTR_IN_FUNCTION
  "-Werror=ptr-in-function"
  "error: .* Function `bar': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "")

add_werror_test(
  WERROR_NO_PTR_IN_FUNCTION
  "-Werror;-Wptr-in-function;-Wno-ptr-in-function"
  ""
  "error: .* Function `bar': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
)

add_werror_test(
  NO_WERROR_PTR_IN_FUNCTION
  "-Werror;-Wptr-in-function;-Wno-error"
  "warning: .* Function `bar': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
  "error: .* Function `bar': `s' is a pointer that is not serializable. Consider annotating the parameter with the direction annotation .-Wptr-in-function."
)

add_werror_test(
  WERROR_FOREIGN_TYPE_PTR
  "-Werror;-Wforeign-type-ptr"
  "error: .* Function `bar': `s' is a pointer of a foreign type `TestStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_werror_test(
  WERROR_EQUAL_TO_FOREIGN_TYPE_PTR
  "-Werror=foreign-type-ptr"
  "error: .* Function `bar': `s' is a pointer of a foreign type `TestStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "")

add_werror_test(
  WERROR_NO_FOREIGN_TYPE_PTR
  "-Werror;-Wforeign-type-ptr;-Wno-foreign-type-ptr"
  ""
  "error: .* Function `bar': `s' is a pointer of a foreign type `TestStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_werror_test(
  NO_WERROR_FOREIGN_TYPE_PTR
  "-Werror;-Wforeign-type-ptr;-Wno-error"
  "warning: .* Function `bar': `s' is a pointer of a foreign type `TestStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
  "error: .* Function `bar': `s' is a pointer of a foreign type `TestStruct' that may not be serializable. Consider defining the type in the EDL file with proper annotations .-Wforeign-type-ptr."
)

add_werror_test(
  WERROR_UNSUPPOTED_ALLOW
  "-Werror;-Wunsupported-allow"
  "error: .* Function `baz': the `allow' syntax is currently unsupported. Ignored .-Wunsupported-allow."
  "")

add_werror_test(
  WERROR_EQUAL_TO_UNSUPPORTED_ALLOW
  "-Werror=unsupported-allow"
  "error: .* Function `baz': the `allow' syntax is currently unsupported. Ignored .-Wunsupported-allow."
  "")
