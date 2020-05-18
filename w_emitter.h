// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef W_EMITTER_H
#define W_EMITTER_H

#include <fstream>

#include "ast.h"
#include "utils.h"

class WEmitter
{
    Edl* edl_;
    std::ofstream& file_;
    bool ecall_;

  public:
    typedef WEmitter& R;
    R out()
    {
        return *this;
    }

    template <typename T>
    R operator<<(const T& t)
    {
        file_ << t << "\n";
        return out();
    }

  public:
    WEmitter(Edl* edl, std::ofstream& file)
        : edl_(edl), file_(file), ecall_(true)
    {
    }

    void get_functions(
        Function* f,
        std::string& alloc_fcn,
        std::string& free_fcn,
        std::string& call)
    {
        if (f->switchless_)
        {
            if (!ecall_)
            {
                alloc_fcn = "oe_allocate_switchless_ocall_buffer";
                free_fcn = "oe_free_switchless_ocall_buffer";
                call = "oe_switchless_call_host_function";
            }
            else
            {
                alloc_fcn = "oe_malloc";
                free_fcn = "free";
                call = "oe_switchless_call_enclave_function";
            }
        }
        else
        {
            if (!ecall_)
            {
                alloc_fcn = "oe_allocate_ocall_buffer";
                free_fcn = "oe_free_ocall_buffer";
                call = "oe_call_host_function";
            }
            else
            {
                alloc_fcn = "oe_malloc";
                free_fcn = "free";
                call = "oe_call_enclave_function";
            }
        }
    }

    void emit(Function* f, bool ecall)
    {
        ecall_ = ecall;
        std::string alloc_fcn;
        std::string free_fcn;
        std::string call;
        get_functions(f, alloc_fcn, free_fcn, call);
        std::string other = ecall ? "enclave" : "host";

        std::string args_t = f->name_ + "_args_t";
        out() << prototype(f, ecall, gen_t()) << "{"
              << "    oe_result_t _result = OE_FAILURE;"
              << "";
        enclave_status_check();
        out() << "    /* Marshalling struct. */"
              << "    " + args_t +
                     " _args, *_pargs_in = NULL, *_pargs_out = NULL;";
        save_deep_copy_pointers(f);
        out() << "    /* Marshalling buffer and sizes. */"
              << "    size_t _input_buffer_size = 0;"
              << "    size_t _output_buffer_size = 0;"
              << "    size_t _total_buffer_size = 0;"
              << "    uint8_t* _buffer = NULL;"
              << "    uint8_t* _input_buffer = NULL;"
              << "    uint8_t* _output_buffer = NULL;"
              << "    size_t _input_buffer_offset = 0;"
              << "    size_t _output_buffer_offset = 0;"
              << "    size_t _output_bytes_written = 0;"
              << "";
        deep_copy_buffer(f);
        out() << "    /* Fill marshalling struct. */"
              << "    memset(&_args, 0, sizeof(_args));";
        fill_marshalling_struct(f);
        out() << ""
              << "    /* Compute input buffer size. Include in and in-out "
                 "parameters. */";
        compute_input_buffer_size(f);
        out() << "    "
              << "    /* Compute output buffer size. Include out and in-out "
                 "parameters. */";
        compute_output_buffer_size(f);
        out()
            << "    "
            << "    /* Allocate marshalling buffer. */"
            << "    _total_buffer_size = _input_buffer_size;"
            << "    OE_ADD_SIZE(_total_buffer_size, _output_buffer_size);"
            << "    _buffer = (uint8_t*)" + alloc_fcn + "(_total_buffer_size);"
            << "    _input_buffer = _buffer;"
            << "    _output_buffer = _buffer + _input_buffer_size;"
            << "    if (_buffer == NULL)"
            << "    {"
            << "        _result = OE_OUT_OF_MEMORY;"
            << "        goto done;"
            << "    }"
            << "    "
            << "    /* Serialize buffer inputs (in and in-out parameters). */";
        serialize_buffer_inputs(f);
        out() << "    "
              << "    /* Copy args structure (now filled) to input buffer. */"
              << "    memcpy(_pargs_in, &_args, sizeof(*_pargs_in));"
              << ""
              << "    /* Call " + other + " function. */"
              << "    if ((_result = " + call + "(";
        if (!gen_t())
            out() << "             enclave,";
        out() << "             " + edl_->name_ + "_fcn_id_" + f->name_ + ","
              << "             _input_buffer,"
              << "             _input_buffer_size,"
              << "             _output_buffer,"
              << "             _output_buffer_size,"
              << "             &_output_bytes_written)) != OE_OK)"
              << "        goto done;"
              << ""
              << "    /* Setup output arg struct pointer. */"
              << "    _pargs_out = (" + args_t + "*)_output_buffer;"
              << "    OE_ADD_SIZE(_output_buffer_offset, sizeof(*_pargs_out));"
              << "    "
              << "    /* Check if the call succeeded. */"
              << "    if ((_result = _pargs_out->_result) != OE_OK)"
              << "        goto done;"
              << "    "
              << "    /* Currently exactly _output_buffer_size bytes must be "
                 "written. */"
              << "    if (_output_bytes_written != _output_buffer_size)"
              << "    {"
              << "        _result = OE_FAILURE;"
              << "        goto done;"
              << "    }"
              << "    "
              << "    /* Unmarshal return value and out, in-out parameters. */";
        if (f->rtype_->tag_ != Void)
            out() << "    *_retval = _pargs_out->_retval;";
        else
            out() << "    /* No return value. */";
        restore_pointers_deep_copy(f);
        unmarshal_outputs(f);
        out() << "";
        propagate_errno(f);
        out() << "    _result = OE_OK;"
              << ""
              << "done:"
              << "    if (_buffer)"
              << "        " + free_fcn + "(_buffer);";
        if (!gen_t())
            out() << "";
        deep_copy_free_pointers(f);
        out() << "    return _result;"
              << "}"
              << "";
    }

    bool gen_t() const
    {
        return !ecall_;
    }

    void enclave_status_check()
    {
        if (gen_t())
            out() << "    /* If the enclave is in crashing/crashed status, new "
                     "OCALL should fail"
                  << "       immediately. */"
                  << "    if (oe_get_enclave_status() != OE_OK)"
                  << "        return oe_get_enclave_status();"
                  << "";
    }

    void fill_marshalling_struct(Function* f)
    {
        for (Decl* p : f->params_)
        {
            std::string lhs = "    _args." + p->name_ + " = ";
            if (p->attrs_)
            {
                std::string ty = mtype_str(p);
                if (ty[0] == '/')
                    ty = replace(ty, "void*", "(void*)");
                else
                    ty = "(" + ty + ")";
                out() << lhs + ty + p->name_ + ";";
                if (p->attrs_->string_ || p->attrs_->wstring_)
                {
                    std::string strlen =
                        p->attrs_->wstring_ ? "oe_wcslen" : "oe_strlen";
                    out() << "    _args." + p->name_ + "_len = (" + p->name_ +
                                 ") ? (" + strlen + "(" + p->name_ +
                                 ") + 1) : 0;";
                }
            }
            else
                out() << lhs + p->name_ + ";";
        }
        if (f->params_.empty())
            out() << "    ";
    }

    void compute_input_buffer_size(Function* f)
    {
        out() << "    OE_ADD_SIZE(_input_buffer_size, sizeof(" + f->name_ +
                     "_args_t));";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && (p->attrs_->in_ || p->attrs_->inout_))
            {
                std::string size = replace(psize(p), "pargs_in->", "_args.");
                out() << "    if (" + p->name_ + ")"
                      << "        OE_ADD_SIZE(_input_buffer_size, " + size +
                             ");";
                empty = false;
            }
        }
        if (empty)
            out() << "    /* There were no corresponding parameters. */";
    }

    void compute_output_buffer_size(Function* f)
    {
        out() << "    OE_ADD_SIZE(_output_buffer_size, sizeof(" + f->name_ +
                     "_args_t));";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && (p->attrs_->out_ || p->attrs_->inout_))
            {
                std::string size = replace(psize(p), "pargs_in->", "_args.");
                out() << "    if (" + p->name_ + ")"
                      << "        OE_ADD_SIZE(_output_buffer_size, " + size +
                             ");";
                empty = false;
            }
        }
        if (empty)
            out() << "    /* There were no corresponding parameters. */";
    }

    void serialize_buffer_inputs(Function* f)
    {
        out() << "    _pargs_in = (" + f->name_ + "_args_t*)_input_buffer;"
              << "    OE_ADD_SIZE(_input_buffer_offset, sizeof(*_pargs_in));";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && (p->attrs_->in_ || p->attrs_->inout_))
            {
                std::string mt = mtype_str(p);
                std::string size = replace(psize(p), "pargs_in->", "_args.");
                std::string write_fcn = p->attrs_->inout_
                                            ? "OE_WRITE_IN_OUT_PARAM"
                                            : "OE_WRITE_IN_PARAM";
                out() << "    if (" + p->name_ + ")"
                      << "        " + write_fcn + "(" + p->name_ + ", " + size +
                             ", " + mt + ");";
                empty = false;
            }
        }
        if (empty)
            out() << "    /* There were no in nor in-out parameters. */";
    }

    void unmarshal_outputs(Function* f)
    {
        std::string check = "OE_CHECK_NULL_TERMINATOR";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && (p->attrs_->out_ || p->attrs_->inout_))
            {
                std::string size = replace(psize(p), "pargs_in->", "_args.");
                std::string read_fcn = p->attrs_->inout_
                                           ? "OE_READ_IN_OUT_PARAM"
                                           : "OE_READ_OUT_PARAM";
                if (p->attrs_->string_ || p->attrs_->wstring_)
                {
                    out() << "    " + check +
                                 (p->attrs_->wstring_ ? "_WIDE(" : "(") +
                                 "_output_buffer + _output_buffer_offset, "
                                 "_args." +
                                 p->name_ + "_len);";
                }
                out() << "    " + read_fcn + "(" + p->name_ + ", (size_t)(" +
                             size + "));";
                empty = false;
            }
        }
        if (empty)
            out() << "    /* There were no out nor in-out parameters. */";
    }

    void propagate_errno(Function* f)
    {
        if (!gen_t())
            return;
        out() << "    /* Retrieve propagated errno from OCALL. */";
        if (f->errno_)
            out() << "    oe_errno = _pargs_out->_ocall_errno;"
                  << "";
        else
            out() << "    /* Errno propagation not enabled. */";
        out() << "";
    }

    void save_deep_copy_pointers(Function* f)
    {
        (void)f;
        if (gen_t())
            out() << "    /* No pointers to save for deep copy. */";
        out() << "";
    }

    void deep_copy_buffer(Function* f)
    {
        (void)f;
        if (!gen_t())
            out() << "    /* Deep copy buffer. */"
                  << "    /* No pointers to save for deep copy. */"
                  << "";
    }

    void restore_pointers_deep_copy(Function* f)
    {
        (void)f;
        out() << "    /* No pointers to restore for deep copy. */";
    }

    void deep_copy_free_pointers(Function* f)
    {
        (void)f;
        if (!gen_t())
            out() << "    /* No `_ptrs` to free for deep copy. */"
                  << "";
    }
};

#endif // W_EMITTER_H
