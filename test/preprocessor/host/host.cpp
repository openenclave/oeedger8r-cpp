// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdio.h>

#include <openenclave/internal/tests.h>
#include "preprocessor_u.h"

int host_ifdef_ocall(int magic)
{
    OE_TEST(magic == 3);
    return 4;
}

int host_else_ocall(int magic)
{
    OE_TEST(magic == 3);
    return 4;
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
        oe_create_preprocessor_enclave(
            argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave) == OE_OK);

    int ret_val = -1;
    // import "import_1.edl"
    OE_TEST(enc_import_1(enclave, &ret_val, 10) == OE_OK);
    OE_TEST(ret_val == 11);

    OE_TEST(enc_import_2(enclave, &ret_val, 20) == OE_OK);
    OE_TEST(ret_val == 21);

    // from "import_2.edl" import *;
    OE_TEST(enc_import_3(enclave, &ret_val, 30) == OE_OK);
    OE_TEST(ret_val == 31);

    OE_TEST(enc_import_4(enclave, &ret_val, 40) == OE_OK);
    OE_TEST(ret_val == 41);

    // from "import_3.edl" import enc_import_5;
    OE_TEST(enc_import_5(enclave, &ret_val, 50) == OE_OK);
    OE_TEST(ret_val == 51);

    // import "import_5.edl"
    OE_TEST(enc_import_9(enclave, &ret_val, 90) == OE_OK);
    OE_TEST(ret_val == 91);

    OE_TEST(enc_ifdef_ocall(enclave) == OE_OK);

    OE_TEST(enc_ifdef_ecall(enclave, &ret_val, 5) == OE_OK);
    OE_TEST(ret_val == 6);

    OE_TEST(enc_ifdef_enum(enclave, &ret_val, Test_Enum_2) == OE_OK);
    OE_TEST(ret_val == 7);

    TestStruct ts;
    ts.num = 8;
    ts.c = 9;
    OE_TEST(enc_ifdef_struct(enclave, &ret_val, ts) == OE_OK);
    OE_TEST(ret_val == 10);

    OE_TEST(enc_else_ocall(enclave) == OE_OK);

    OE_TEST(enc_else_ecall(enclave, &ret_val, 5) == OE_OK);
    OE_TEST(ret_val == 6);

    OE_TEST(enc_else_enum(enclave, &ret_val, Test_Else_Enum_3) == OE_OK);
    OE_TEST(ret_val == 7);

    TestElseStruct elsets;
    elsets.num = 8;
    elsets.c = 9;
    OE_TEST(enc_else_struct(enclave, &ret_val, elsets) == OE_OK);
    OE_TEST(ret_val == 10);

    OE_TEST(enc_nested_ifdef_ecall(enclave, &ret_val, 123) == OE_OK);
    OE_TEST(ret_val == 456);

    // Exercise complex preprocessor usage.
    int p = 1;
    OE_TEST(enc_complex_ecall1(enclave, &p, p) == OE_OK);

    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    printf("=== passed all tests (preprocessor)\n");
}
