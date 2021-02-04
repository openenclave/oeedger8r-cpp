// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdio.h>

#include <openenclave/internal/tests.h>
#include "safe_math_u.h"

int host_add_overflow(char* buf, size_t size)
{
    (void)buf;
    (void)size;
    return 0;
}

int host_mul_overflow(int* buf, size_t count)
{
    (void)buf;
    (void)count;
    return 0;
}

int main(int argc, char** argv)
{
    oe_enclave_t* enclave = NULL;

    const uint32_t flags = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH\n", argv[0]);
        return 1;
    }

    OE_TEST(
        oe_create_safe_math_enclave(
            argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave) == OE_OK);

    int ret_val = -1;
    char buf[10];
    OE_TEST(
        enc_add_overflow(enclave, &ret_val, buf, UINT64_MAX) ==
        OE_INTEGER_OVERFLOW);

    OE_TEST(
        enc_mul_overflow(enclave, &ret_val, (int*)buf, UINT64_MAX) ==
        OE_INTEGER_OVERFLOW);

    {
        CountSizeParamStruct s;
        s.ptr = (uint64_t*)buf;
        s.size = UINT64_MAX;
        s.count = UINT64_MAX;
        OE_TEST(
            enc_deepcopy_in_overflow(enclave, &ret_val, &s) ==
            OE_INTEGER_OVERFLOW);
    }

    {
        CountSizeParamStruct s;
        s.ptr = (uint64_t*)buf;
        s.size = UINT64_MAX;
        s.count = UINT64_MAX;
        OE_TEST(
            enc_deepcopy_inout_overflow(enclave, &ret_val, &s) ==
            OE_INTEGER_OVERFLOW);
    }

    {
        CountSizeParamStruct s;
        OE_TEST(
            enc_deepcopy_out_overflow(enclave, &ret_val, &s) ==
            OE_INTEGER_OVERFLOW);
    }

    OE_TEST(enc_host_test(enclave) == OE_OK);

    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    printf("=== passed all tests (safe_math)\n");
}
