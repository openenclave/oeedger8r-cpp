# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_subdirectory(enc)
add_subdirectory(host)

add_test(oeedger8r_test_safe_math host/oeedger8r_safe_math_host
         enc/oeedger8r_safe_math_enc)
