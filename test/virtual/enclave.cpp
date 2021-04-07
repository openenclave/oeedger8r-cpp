// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/edger8r/enclave.h>
#include "enclave_impl.h"

#include <map>

extern "C"
{
    oe_enclave_t* _enclave;

    extern "C" oe_ecall_func_t oe_ecalls_table[];
    extern "C" size_t oe_ecalls_table_size;

    OE_EXPORT
    void set_enclave_object(oe_enclave_t* enclave)
    {
        _enclave = enclave;
        _enclave->_ecall_table = oe_ecalls_table;
        _enclave->_num_ecalls = static_cast<uint32_t>(oe_ecalls_table_size);
    }

    bool oe_is_within_enclave(const void* ptr, uint64_t size)
    {
        return _enclave->is_within_enclave(ptr, size);
    }

    bool oe_is_outside_enclave(const void* ptr, uint64_t size)
    {
        return _enclave->is_outside_enclave(ptr, size);
    }

    oe_result_t oe_get_enclave_status(void)
    {
        return _enclave->status;
    }

    size_t strlen(const char*);

    void* oe_allocate_ocall_buffer(size_t size)
    {
        return _enclave->malloc(size);
    }

    void oe_free_ocall_buffer(void* ptr)
    {
        return _enclave->free(ptr);
    }

    void* oe_allocate_switchless_ocall_buffer(size_t size)
    {
        return _enclave->malloc(size);
    }

    void oe_free_switchless_ocall_buffer(void* ptr)
    {
        return _enclave->free(ptr);
    }

    void* oe_malloc(size_t size)
    {
        return _enclave->malloc(size);
    }

    void oe_free(void* ptr)
    {
        return _enclave->free(ptr);
    }

    oe_result_t oe_call_host_function(
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

        /* Simulate the args->deepcopy_out_buffer passing. */
        oe_call_args_t* args = static_cast<oe_call_args_t*>(output_buffer);
        if (args->deepcopy_out_buffer && args->deepcopy_out_buffer_size)
        {
            uint8_t* enclave_buffer =
                (uint8_t*)oe_malloc(args->deepcopy_out_buffer_size);
            memcpy(
                enclave_buffer,
                args->deepcopy_out_buffer,
                args->deepcopy_out_buffer_size);
            /* oe_host_free. */
            free(args->deepcopy_out_buffer);
            args->deepcopy_out_buffer = enclave_buffer;
        }
        return *reinterpret_cast<oe_result_t*>(output_buffer);
    }

    oe_result_t oe_switchless_call_host_function(
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

        /* Emulate the args->deepcopy_out_buffer passing. */
        oe_call_args_t* args = static_cast<oe_call_args_t*>(output_buffer);
        if (args->deepcopy_out_buffer && args->deepcopy_out_buffer_size)
        {
            uint8_t* enclave_buffer =
                (uint8_t*)oe_malloc(args->deepcopy_out_buffer_size);
            memcpy(
                enclave_buffer,
                args->deepcopy_out_buffer,
                args->deepcopy_out_buffer_size);
            /* oe_host_free. */
            free(args->deepcopy_out_buffer);
            args->deepcopy_out_buffer = enclave_buffer;
        }
        return *reinterpret_cast<oe_result_t*>(output_buffer);
    }

    // Required by tests
    int strcmp(const char* s1, const char* s2);

    int oe_strcmp(const char* s1, const char* s2)
    {
        return strcmp(s1, s2);
    }

    thread_local int __oe_errno;

    int* __oe_errno_location(void)
    {
        return &__oe_errno;
    }
}
