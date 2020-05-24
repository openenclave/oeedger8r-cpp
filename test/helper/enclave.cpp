// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/edger8r/enclave.h>
#include "enclave_impl.h"

#include <map>

thread_local oe_enclave_t* _enclave;

extern "C" bool oe_is_within_enclave(const void* ptr, uint64_t size)
{
    return _enclave->is_within_enclave(ptr, size);
}

extern "C" oe_result_t oe_get_enclave_status(void)
{
    return _enclave->status;
}

extern "C" size_t strlen(const char*);

extern "C" size_t oe_strlen(const char* str)
{
    return strlen(str);
}

extern "C" void* oe_allocate_ocall_buffer(size_t size)
{
    return _enclave->malloc(size);
}

extern "C" void oe_free_ocall_buffer(void* ptr)
{
    return _enclave->free(ptr);
}

extern "C" oe_result_t oe_call_host_function(
    size_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    _enclave->_ocall_table[function_id](
        reinterpret_cast<const uint8_t*>(input_buffer),
        input_buffer_size,
        reinterpret_cast<uint8_t*>(output_buffer),
        output_buffer_size,
        output_bytes_written);
    return *reinterpret_cast<oe_result_t*>(output_buffer);
}
