#ifndef ENCLAVE_IMPL_H
#define ENCLAVE_IMPL_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

#include <cstdlib>
#include <map>

#define OE_ECALL_ID_NULL OE_UINT64_MAX
/* Temporily set value. */
#define OE_MAX_ECALLS 256

typedef void (*oe_ocall_func_t)(
    const uint8_t* input_buffer,
    size_t input_buffer_size,
    uint8_t* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written);

typedef void (*oe_ecall_func_t)(
    const uint8_t* input_buffer,
    size_t input_buffer_size,
    uint8_t* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written);

/**
 * Type of ecall information
 */
typedef struct _oe_ecall_id_t
{
    uint64_t id;
} oe_ecall_id_t;

typedef struct _oe_call_args_t
{
    oe_result_t result;
    uint8_t* deepcopy_out_buffer;
    size_t deepcopy_out_buffer_size;
} oe_call_args_t;

struct _oe_enclave
{
    oe_result_t status;
    std::map<void*, void*> _allocated_memory;
    const oe_ocall_func_t* _ocall_table;
    uint32_t _num_ocalls;
    const oe_ecall_func_t* _ecall_table;
    uint32_t _num_ecalls;
    oe_ecall_id_t _ecall_id_table[OE_MAX_ECALLS];
    void (*_set_enclave)(oe_enclave_t*);
    void* _lib_handle;

    _oe_enclave(
        const oe_ocall_func_t* ocall_table,
        uint32_t num_ocalls,
        uint32_t num_ecalls)
    {
        status = OE_OK;
        _ocall_table = ocall_table;
        _num_ocalls = num_ocalls;
        _ecall_table = nullptr;
        _num_ecalls = 0;
        _set_enclave = nullptr;
        _lib_handle = nullptr;
        for (int i = 0; i < OE_MAX_ECALLS; i++)
        {
            _ecall_id_table[i].id = OE_ECALL_ID_NULL;
        }
    }

    void* malloc(uint64_t size)
    {
        void* ptr = ::malloc(size);
        _allocated_memory[ptr] = (uint8_t*)ptr + size;
        return ptr;
    }

    void free(void* ptr)
    {
        if (_allocated_memory.find(ptr) != _allocated_memory.end())
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
            if (ptr >= p.first && end <= p.second)
                return true;
        }

        return false;
    }

    bool is_outside_enclave(const void* ptr, uint64_t size)
    {
        if (!ptr)
            return false;

        const void* end = static_cast<const uint8_t*>(ptr) + size;
        for (auto p : _allocated_memory)
        {
            if (ptr >= p.first && ptr <= p.second)
                return false;
            if (end >= p.first && end <= p.second)
                return false;
        }

        return true;
    }
};

#endif
