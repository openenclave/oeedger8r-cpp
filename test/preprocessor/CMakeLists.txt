# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_subdirectory(enc)
add_subdirectory(host)

add_test(oeedger8r_preprocessor host/oeedger8r_preprocessor_host
         enc/oeedger8r_preprocessor_enc)
