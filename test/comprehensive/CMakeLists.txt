# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_subdirectory(enc)
add_subdirectory(host)

add_test(oeedger8r_comprehensive host/oeedger8r_comprehensive_host
         enc/oeedger8r_comprehensive_enc)
