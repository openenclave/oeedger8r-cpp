// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/tests.h>

#include "all_u.h"

void test_security_edl_ecalls(oe_enclave_t* enclave)
{
    int* ptr;
    OE_TEST(security_get_secret_ptr(enclave, &ptr) == OE_OK);

    OE_TEST(security_ecall_test1(enclave, ptr) != OE_OK);

    int ret_value;
    OE_TEST(security_ecall_test2(enclave, &ret_value, NULL) == OE_OK);
    OE_TEST(ret_value == 0);
    OE_TEST(security_ecall_test3(enclave, &ret_value, NULL) == OE_OK);
    OE_TEST(ret_value == 0);

    OE_TEST(security_ecall_test4(enclave, &ret_value, NULL) == OE_OK);
    OE_TEST(ret_value == 0);

    printf("=== test_security_edl_ecalls passed\n");
}
