# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_subdirectory(enc1)
add_subdirectory(enc2)
add_subdirectory(host)

add_test(oeedger8r_prefix host/oeedger8r_prefix_host enc1/oeedger8r_enc1
         enc2/oeedger8r_enc2)
