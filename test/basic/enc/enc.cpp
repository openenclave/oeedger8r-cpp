// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/internal/tests.h>
#include "basic_t.h"

#include <stdio.h>
#include <string.h>

int enc_hello(const char* msg, int* out_val)
{
    printf("enc: Hello, World\n");
    printf("enc: host says: '%s'\n", msg);
    OE_TEST(strcmp(msg, "I'm host!") == 0);
    int ret_val = -1;
    OE_TEST(host_hello(&ret_val, "I'm enclave!", out_val) == OE_OK);
    OE_TEST(ret_val == 0);
    return *out_val;
}
