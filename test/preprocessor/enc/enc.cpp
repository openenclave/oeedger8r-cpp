// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/internal/tests.h>
#include "preprocessor_t.h"

#include <stdio.h>

int enc_import_1(int magic)
{
    OE_TEST(magic == 10);
    return 11;
}

int enc_import_2(int magic)
{
    OE_TEST(magic == 20);
    return 21;
}

int enc_import_3(int magic)
{
    OE_TEST(magic == 30);
    return 31;
}

int enc_import_4(int magic)
{
    OE_TEST(magic == 40);
    return 41;
}

int enc_import_5(int magic)
{
    OE_TEST(magic == 50);
    return 51;
}

int enc_import_9(int magic)
{
    OE_TEST(magic == 90);
    return 91;
}

void enc_ifdef_ocall()
{
    int ret_val = -1;
    OE_TEST(host_ifdef_ocall(&ret_val, 3) == OE_OK);
    OE_TEST(ret_val == 4);
}

int enc_ifdef_ecall(int magic)
{
    OE_TEST(magic = 5);
    return 6;
}

int enc_ifdef_enum(TestEnum value)
{
    OE_TEST(value == Test_Enum_2);
    return 7;
}

int enc_ifdef_struct(TestStruct st)
{
    OE_TEST(st.num == 8);
    OE_TEST(st.c == 9);
    return 10;
}

void enc_else_ocall()
{
    int ret_val = -1;
    OE_TEST(host_else_ocall(&ret_val, 3) == OE_OK);
    OE_TEST(ret_val == 4);
}

int enc_else_ecall(int magic)
{
    OE_TEST(magic = 5);
    return 6;
}

int enc_else_enum(TestElseEnum value)
{
    OE_TEST(value == Test_Else_Enum_3);
    return 7;
}

int enc_else_struct(TestElseStruct st)
{
    OE_TEST(st.num == 8);
    OE_TEST(st.c == 9);
    return 10;
}

int enc_nested_ifdef_ecall(int magic)
{
    OE_TEST(magic == 123);
    return 456;
}

void enc_complex_ecall1(int* a, int n)
{
    OE_TEST(*a == 1);
    OE_TEST(n == 1);
}
