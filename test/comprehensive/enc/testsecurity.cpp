// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.
#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>

#include "all_t.h"

int myint1;
int* security_get_secret_ptr()
{
    return &myint1;
}

void security_ecall_test1(int* ptr)
{
    OE_UNUSED(ptr);
    return;
}

int security_ecall_test2(int* ptr)
{
    if (ptr == NULL)
        return 0;
    return -1;
}

int security_ecall_test3(int* ptr)
{
    if (ptr == NULL)
        return 0;
    return -1;
}

int security_ecall_test4(int* ptr)
{
    if (ptr == NULL)
        return 0;
    return -1;
}
