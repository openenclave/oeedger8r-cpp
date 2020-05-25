// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <stdio.h>

#include <openenclave/internal/tests.h>
#include "basic_u.h"

#define MAGIC_VAL 0x11223344

int host_hello(const char* msg, int* out_val)
{
    printf("host: Hello, World\n");
    printf("host: enc says: '%s'\n", msg);
    OE_TEST(strcmp(msg, "I'm enclave!") == 0);
    *out_val = MAGIC_VAL;
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
        oe_create_basic_enclave(
            argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave) == OE_OK);

    int ret_val = -1;
    int out_val = 0;
    OE_TEST(enc_hello(enclave, &ret_val, "I'm host!", &out_val) == OE_OK);
    OE_TEST(ret_val == MAGIC_VAL);
    OE_TEST(out_val == MAGIC_VAL);

    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    printf("=== passed all tests (basic)\n");
}
