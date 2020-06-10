// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/internal/tests.h>
#include "enc1_t.h"

void enc_ecall1(MyStruct s)
{
    OE_TEST(s.x == 5);
    OE_TEST(s.y == 6);
    OE_TEST(host_ocall1(s) == OE_OK);
}

void enc_ecall2(MyUnion u)
{
    OE_TEST(u.y == 7);
    OE_TEST(host_ocall2(u) == OE_OK);
}

void enc_ecall3(MyEnum e)
{
    OE_TEST(e == GREEN);
    OE_TEST(host_ocall3(e) == OE_OK);
}
