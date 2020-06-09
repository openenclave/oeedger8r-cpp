// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdio.h>

#include <openenclave/internal/tests.h>

// Ensure that there are no multiple definition errors for user defined types.
#include "enc1_u.h"
#include "enc2_u.h"

MyStruct g_s = {5, 6};
MyUnion g_u = {7};
MyEnum g_e = GREEN;

void host_ocall1(MyStruct s)
{
    OE_TEST(s.x == g_s.x);
    OE_TEST(s.y == g_s.y);
}

void host_ocall2(MyUnion u)
{
    OE_TEST(u.y == g_u.y);
}

void host_ocall3(MyEnum e)
{
    OE_TEST(e == g_e);
}

int main(int argc, char** argv)
{
    oe_enclave_t* enc1 = NULL;
    oe_enclave_t* enc2 = NULL;

    const uint32_t flags = 0;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ENCLAVE1_PATH ENCLAVE2_PATH\n", argv[0]);
        return 1;
    }

    OE_TEST(
        oe_create_enc1_enclave(
            argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enc1) == OE_OK);

    OE_TEST(
        oe_create_enc2_enclave(
            argv[2], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enc2) == OE_OK);

    // Call functions in enclave 1.
    OE_TEST(enc_ecall1(enc1, g_s) == OE_OK);
    OE_TEST(enc_ecall2(enc1, g_u) == OE_OK);
    OE_TEST(enc_ecall3(enc1, g_e) == OE_OK);

    // Change values for enclave 2.
    g_s = {8, 9};
    g_u.y = 10;
    OE_TEST(enc2_enc_ecall1(enc2, g_s) == OE_OK);
    OE_TEST(enc2_enc_ecall2(enc2, g_u) == OE_OK);

    OE_TEST(oe_terminate_enclave(enc1) == OE_OK);
    OE_TEST(oe_terminate_enclave(enc2) == OE_OK);

    printf("=== passed all tests (prefix)\n");
}
