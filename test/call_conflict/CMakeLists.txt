# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_subdirectory(enc1)
add_subdirectory(enc2)
add_subdirectory(host)

add_test(oeedger8r_call_conflict host/oeedger8r_call_conflict_host
         enc1/oeedger8r_call_conflict_enc1 enc2/oeedger8r_call_conflict_enc2)
