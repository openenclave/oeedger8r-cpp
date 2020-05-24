#ifndef ENCLAVE_IMPL_H
#define ENCLAVE_IMPL_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

#include <map>

typedef void (*oe_ocall_func_t)(
    const uint8_t* input_buffer,
    size_t input_buffer_size,
    uint8_t* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written);

struct _oe_enclave
{
    oe_result_t status;
    std::map<void*, void*> _allocated_memory;
    const oe_ocall_func_t* _ocall_table;
    uint32_t _num_ocalls;

    _oe_enclave(const oe_ocall_func_t* ocall_table, uint32_t num_ocalls)
    {
        status = OE_OK;
        _ocall_table = ocall_table;
        _num_ocalls = num_ocalls;
    }

    void* malloc(uint64_t size)
    {
        void* ptr = ::malloc(size);
        _allocated_memory[ptr] = (uint8_t*)ptr + size;
        return ptr;
    }

    void free(void* ptr)
    {
        // TODO: Assert map element.
        _allocated_memory.erase(ptr);
        ::free(ptr);
    }

    bool is_within_enclave(const void* ptr, uint64_t size)
    {
        if (!ptr)
            return false;

        const void* end = static_cast<const uint8_t*>(ptr) + size;
        for (auto p : _allocated_memory)
        {
            if (ptr >= p.second)
                continue;
            if (end <= p.first)
                continue;
            return true;
        }

        return false;
    }
};

OE_EXPORT
extern thread_local oe_enclave_t* _enclave;

#endif
