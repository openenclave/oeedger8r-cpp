// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <openenclave/edger8r/host.h>
#include <mutex>
#include <string>

#include "enclave_impl.h"

/* Temporily set value. */
#define OE_MAX_ECALLS 256

extern "C"
{
    const char* _ecall_table[OE_MAX_ECALLS];
    uint32_t _ecall_table_size;
    std::mutex oe_lock;
    thread_local oe_enclave_t* _enclave;

    // Get the global ecall id from the _ecall_table.
    oe_result_t oe_get_global_ecall_id_by_name(
        const char* name,
        uint64_t* global_id)
    {
        oe_result_t result = OE_NOT_FOUND;
        uint64_t i;

        oe_lock.lock();
        if (!global_id)
        {
            result = OE_INVALID_PARAMETER;
            goto done;
        }

        for (i = 0; i < _ecall_table_size; i++)
        {
            if (strcmp(_ecall_table[i], name) == 0)
            {
                *global_id = i;
                result = OE_OK;
                break;
            }
        }

        /* If the name is not found, adding it to the table. */
        if (result == OE_NOT_FOUND)
        {
            if (_ecall_table_size >= OE_MAX_ECALLS)
            {
                result = OE_OUT_OF_BOUNDS;
                goto done;
            }

            _ecall_table[_ecall_table_size] = name;
            *global_id = _ecall_table_size;
            _ecall_table_size++;
            result = OE_OK;
        }

    done:
        oe_lock.unlock();
        return result;
    }

    oe_result_t oe_get_enclave_function_id(
        oe_enclave_t* enclave,
        uint64_t* global_id,
        const char* name,
        uint64_t* id)
    {
        oe_result_t result = OE_FAILURE;

        if (!enclave || !global_id || !name || !id)
        {
            result = OE_INVALID_PARAMETER;
            goto done;
        }

        if (*global_id == OE_GLOBAL_ECALL_ID_NULL)
        {
            if ((result = oe_get_global_ecall_id_by_name(name, global_id)) !=
                OE_OK)
                goto done;
        }

        if (*global_id >= OE_MAX_ECALLS)
        {
            result = OE_OUT_OF_BOUNDS;
            goto done;
        }

        /* Look-up the ecall id from the per-enclave table. */
        *id = enclave->_ecall_id_table[*global_id].id;

        result = OE_OK;
    done:

        return result;
    }

    static oe_result_t oe_host_register_enclave_functions(
        oe_enclave_t* enclave,
        const oe_ecall_info_t* ecall_info_table,
        uint32_t num_ecalls)
    {
        oe_result_t result = OE_UNEXPECTED;

        if (!enclave)
        {
            result = OE_INVALID_PARAMETER;
            goto done;
        }

        /*  Nothing to add when the table is empty, fall through. */
        if (!ecall_info_table || !num_ecalls)
        {
            result = OE_OK;
            goto done;
        }

        uint32_t i;
        for (i = 0; i < num_ecalls; i++)
        {
            uint64_t global_id = OE_GLOBAL_ECALL_ID_NULL;
            const char* name = ecall_info_table[i].name;
            uint64_t local_id = i;

            /* Assign a proper global id based on the global __ecall_table. */
            if ((oe_get_global_ecall_id_by_name(name, &global_id)) != OE_OK)
                continue;

            enclave->_ecall_id_table[global_id].id = local_id;
        }
        result = OE_OK;

    done:

        return result;
    }

    extern "C" oe_result_t oe_call_enclave_function(
        oe_enclave_t* enclave,
        uint64_t* global_id,
        const char* name,
        const void* input_buffer,
        size_t input_buffer_size,
        void* output_buffer,
        size_t output_buffer_size,
        size_t* output_bytes_written)
    {
        oe_result_t result = OE_FAILURE;
        oe_enclave_t* previous_enclave = _enclave;
        uint64_t function_id = OE_ECALL_ID_NULL;
        _enclave = enclave;
        enclave->_set_enclave(enclave);

        if (!_enclave->is_outside_enclave(input_buffer, input_buffer_size) ||
            !_enclave->is_outside_enclave(output_buffer, output_buffer_size) ||
            !global_id)
        {
            result = OE_INVALID_PARAMETER;
            goto done;
        }

        /*
         * Look up the function id from the per-enclave table based on the
         * global id. The global id is defined as a static variable in the
         * oeedger8r-generated code. The function initializes the global id in
         * the first invocation and uses the cached global id for the subsequent
         * invocations.
         */
        if (oe_get_enclave_function_id(
                enclave, global_id, name, &function_id) != OE_OK)
            goto done;

        {
            void* enc_input_buffer = enclave->malloc(input_buffer_size);
            memcpy(enc_input_buffer, input_buffer, input_buffer_size);
            void* enc_output_buffer = enclave->malloc(output_buffer_size);
            memset(enc_output_buffer, 0, output_buffer_size);
            size_t* enc_output_bytes_written =
                (size_t*)enclave->malloc(sizeof(size_t));

            if (function_id >= enclave->_num_ecalls)
            {
                result = OE_OUT_OF_BOUNDS;
                goto done;
            }
            enclave->_ecall_table[function_id](
                static_cast<const uint8_t*>(enc_input_buffer),
                input_buffer_size,
                static_cast<uint8_t*>(enc_output_buffer),
                output_buffer_size,
                enc_output_bytes_written);

            *output_bytes_written = *enc_output_bytes_written;

            /* Emulate the args->deepcopy_out_buffer passing. */
            oe_call_args_t* args =
                static_cast<oe_call_args_t*>(enc_output_buffer);
            if (args->deepcopy_out_buffer && args->deepcopy_out_buffer_size)
            {
                uint8_t* host_buffer =
                    (uint8_t*)malloc(args->deepcopy_out_buffer_size);
                memcpy(
                    host_buffer,
                    args->deepcopy_out_buffer,
                    args->deepcopy_out_buffer_size);
                enclave->free(args->deepcopy_out_buffer);
                args->deepcopy_out_buffer = host_buffer;
            }

            memcpy(output_buffer, enc_output_buffer, output_buffer_size);

            enclave->free(enc_output_bytes_written);
            enclave->free(enc_output_buffer);
            enclave->free(enc_input_buffer);
            result = *(oe_result_t*)output_buffer;
        }

    done:
        _enclave = previous_enclave;

        if (result == OE_INVALID_PARAMETER)
            printf("ecall returned OE_INVALID_PARAMETER\n");

        return result;
    }

    oe_result_t oe_create_enclave(
        const char* path,
        oe_enclave_type_t type,
        uint32_t flags,
        const oe_enclave_setting_t* settings,
        uint32_t setting_count,
        const oe_ocall_func_t* ocall_table,
        uint32_t num_ocalls,
        const oe_ecall_info_t* ecall_info_table,
        uint32_t num_ecalls,
        oe_enclave_t** enclave)
    {
        OE_UNUSED(path);
        OE_UNUSED(type);
        OE_UNUSED(flags);
        OE_UNUSED(settings);
        OE_UNUSED(setting_count);

        oe_enclave_t* enc =
            new _oe_enclave(ocall_table, num_ocalls, num_ecalls);
        printf("Loading virtual enclave %s\n", path);
#if _WIN32
        std::string path_with_ext = std::string(path) + ".dll";
        HMODULE h = LoadLibraryExA(path, NULL, 0);
        if (!h)
            h = LoadLibraryExA(path_with_ext.c_str(), NULL, 0);
        *(void**)&enc->_set_enclave = GetProcAddress(h, "set_enclave_object");
#else
        std::string path_with_ext = std::string(path) + ".so";

        void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!h)
            h = dlopen(path_with_ext.c_str(), RTLD_NOW | RTLD_LOCAL);
        *(void**)&enc->_set_enclave = dlsym(h, "set_enclave_object");
#endif
        if (!h)
        {
            fprintf(
                stderr,
                "fatal error: could not load shared library %s\n",
                path);
            exit(1);
        }
        if (!enc->_set_enclave)
        {
            fprintf(
                stderr,
                "fatal error: could not find _set_enclave function in %s\n",
                path);
            exit(1);
        }

        if (oe_host_register_enclave_functions(
                enc, ecall_info_table, num_ecalls) != OE_OK)
            return OE_FAILURE;

        enc->_lib_handle = h;
        *enclave = enc;

        return OE_OK;
    }

    oe_result_t oe_terminate_enclave(oe_enclave_t* enclave)
    {
        delete enclave;
        return OE_OK;
    }

    uint32_t oe_get_create_flags(void)
    {
        return 0;
    }
}
