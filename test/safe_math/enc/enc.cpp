// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/internal/tests.h>
#include "safe_math_t.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int enc_add_overflow(char* buf, size_t size)
{
    (void)buf;
    (void)size;
    return 0;
}

int enc_mul_overflow(int* buf, size_t count)
{
    (void)buf;
    (void)count;
    return 0;
}

int enc_deepcopy_in_overflow(CountSizeParamStruct* s)
{
    (void)s;
    return 0;
}

int enc_deepcopy_inout_overflow(CountSizeParamStruct* s)
{
    (void)s;
    return 0;
}

int enc_deepcopy_out_overflow(CountSizeParamStruct* s)
{
    s->size = UINT64_MAX;
    s->count = UINT64_MAX;
    s->ptr = (uint64_t*)malloc(10);
    return 0;
}

void enc_host_test()
{
    char* buf;
    int ret_val = -1;
    OE_TEST(
        host_add_overflow(&ret_val, buf, OE_UINT64_MAX) == OE_INTEGER_OVERFLOW);
    OE_TEST(
        host_mul_overflow(&ret_val, (int*)buf, OE_UINT64_MAX) ==
        OE_INTEGER_OVERFLOW);
}
