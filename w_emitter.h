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

    void emit(Function* f, bool ecall, const std::string& prefix = "")
    {
        ecall_ = ecall;
        std::string alloc_fcn;
        std::string free_fcn;
        std::string call;
        get_functions(f, alloc_fcn, free_fcn, call);
        std::string other = ecall ? "enclave" : "host";

        std::string args_t = f->name_ + "_args_t";
        out() << prototype(f, ecall, gen_t(), prefix) << "{"
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

    void add_size_deep_copy(
        const std::string& parent_condition,
        const std::string& parent_expr,
        const std::string& buffer_size,
        Decl* parent_prop,
        int level,
        std::string indent = "    ")
    {
        UserType* ut = get_user_type_for_deep_copy(edl_, parent_prop);
        if (!ut)
            return;
        iterate_deep_copyable_fields(ut, [&](Decl* prop) {
            std::string op = *parent_expr.rbegin() == ']' ? "." : "->";
            std::string expr = parent_expr + op + prop->name_;
            std::string size = psize(prop, "_args." + parent_expr + op);
            std::string cond = parent_condition + " && " + expr;
            out() << indent + "if (" + cond + ")"
                  << indent + "    OE_ADD_SIZE(" + buffer_size + ", " + size +
                         ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count = count_attr_str(prop->attrs_->count_, "_args.");

            if (count == "1" || count == "")
            {
                add_size_deep_copy(
                    cond, expr, buffer_size, prop, level + 1, indent);
            }
            else
            {
                std::string idx = "_i_" + to_str(level);
                std::string expr =
                    parent_expr + op + prop->name_ + "[" + idx + "]";
                out() << indent + "for (size_t " + idx + " = 0; " + idx +
                             " < " + count + "; " + idx + "++)"
                      << indent + "{";
                add_size_deep_copy(
                    cond, expr, buffer_size, prop, level + 1, indent + "    ");
                out() << indent + "}";
            }
        });
    }

    void compute_buffer_size(Function* f, bool input)
    {
        std::string buffer_size =
            input ? "_input_buffer_size" : "_output_buffer_size";
        out() << "    OE_ADD_SIZE(" + buffer_size + ", sizeof(" + f->name_ +
                     "_args_t));";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (!p->attrs_)
                continue;

            if (!p->attrs_->inout_ &&
                !(input ? p->attrs_->in_ : p->attrs_->out_))
                continue;

            std::string size = psize(p, "_args.");
            out() << "    if (" + p->name_ + ")"
                  << "        OE_ADD_SIZE(" + buffer_size + ", " + size + ");";
            empty = false;

            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut)
                continue;

            std::string count = count_attr_str(p->attrs_->count_, "_args.");

            if (count == "1" || count == "")
            {
                std::string cond = p->name_;
                std::string expr = p->name_;
                add_size_deep_copy(cond, expr, buffer_size, p, 2, "    ");
            }
            else
            {
                std::string cond = p->name_;
                std::string expr = p->name_ + "[_i_1]";
                out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                             "; _i_1++)"
                      << "    {";
                add_size_deep_copy(cond, expr, buffer_size, p, 2, "        ");
                out() << "    }";
            }
        }
        if (empty)
            out() << "    /* There were no corresponding parameters. */";
    }

    void compute_input_buffer_size(Function* f)
    {
        compute_buffer_size(f, true);
    }

    void compute_output_buffer_size(Function* f)
    {
        compute_buffer_size(f, false);
    }

    void serialize_pointers_deep_copy(
        const std::string& parent_condition,
        const std::string& parent_expr,
        const std::string& cmd,
        Decl* parent_prop,
        int level,
        std::string indent = "    ")
    {
        UserType* ut = get_user_type_for_deep_copy(edl_, parent_prop);
        if (!ut)
            return;
        iterate_deep_copyable_fields(ut, [&](Decl* prop) {
            std::string op = *parent_expr.rbegin() == ']' ? "." : "->";
            std::string expr = parent_expr + op + prop->name_;
            std::string size = psize(prop, "_args." + parent_expr + op);
            std::string cond = parent_condition + " && " + expr;
            std::string mt = mtype_str(prop);
            out() << indent + "if (" + cond + ")"
                  << indent + "    " + cmd + "(" + expr + ", " + size + ", " +
                         mt + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count = count_attr_str(prop->attrs_->count_, "_args.");

            if (count == "1" || count == "")
            {
                serialize_pointers_deep_copy(
                    cond, expr, cmd, prop, level + 1, indent);
            }
            else
            {
                std::string idx = "_i_" + to_str(level);
                std::string expr =
                    parent_expr + op + prop->name_ + "[" + idx + "]";
                out() << indent + "for (size_t " + idx + " = 0; " + idx +
                             " < " + count + "; " + idx + "++)"
                      << indent + "{";
                serialize_pointers_deep_copy(
                    cond, expr, cmd, prop, level + 1, indent + "    ");
                out() << indent + "}";
            }
        });
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
                std::string size = psize(p, "_args.");
                std::string cmd = p->attrs_->inout_ ? "OE_WRITE_IN_OUT_PARAM"
                                                    : "OE_WRITE_IN_PARAM";
                out() << "    if (" + p->name_ + ")"
                      << "        " + cmd + "(" + p->name_ + ", " + size +
                             ", " + mt + ");";
                empty = false;

                UserType* ut = get_user_type_for_deep_copy(edl_, p);
                if (!ut)
                    continue;

                std::string count = count_attr_str(p->attrs_->count_, "_args.");

                if (count == "1" || count == "")
                {
                    std::string cond = p->name_;
                    std::string expr = p->name_;
                    serialize_pointers_deep_copy(cond, expr, cmd, p, 2, "    ");
                }
                else
                {
                    std::string cond = p->name_;
                    std::string expr = p->name_ + "[_i_1]";
                    out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                                 "; _i_1++)"
                          << "    {";
                    serialize_pointers_deep_copy(
                        cond, expr, cmd, p, 2, "        ");
                    out() << "    }";
                }
            }
        }
        if (empty)
            out() << "    /* There were no in nor in-out parameters. */";
    }

    void unmarshal_deep_copy(
        Decl* p,
        std::string parent_expr,
        const std::string& indent,
        const std::string& cmd,
        int level = 1)
    {
        std::string expr = parent_expr + p->name_;
        UserType* ut = get_user_type_for_deep_copy(edl_, p);
        if (!ut)
        {
            return;
        }
        // Deep copied structure. Unmarshal individual fields.

        out() << indent + "if (" + expr + ")" << indent + "{";
        {
            if (!parent_expr.empty())
                parent_expr += ".";
            std::string size = psize(p, parent_expr);
            std::string p_type = atype_str(p->type_);
            out()
                << indent + "    " + p_type + " _rhs = (" + p_type +
                       ") (_output_buffer + _output_buffer_offset); (void)_rhs;"
                << indent + "    OE_ADD_SIZE(_output_buffer_offset, (size_t)" +
                       size + ");";

            std::string count = count_attr_str(p->attrs_->count_, parent_expr);
            std::string idx = "_i_" + to_str(level);

            out() << indent + "    for (size_t " + idx + " = 0; " + idx +
                         " < " + count + "; " + idx + "++)"
                  << indent + "    {";

            for (Decl* field : ut->fields_)
            {
                // TODO: Do not copy over size.
                if (field->type_->tag_ != Ptr || !field->attrs_ ||
                    field->attrs_->user_check_)
                {
                    std::string prop_val =
                        expr + "[" + idx + "]." + field->name_;
                    out() << indent + "        " + prop_val + " = _rhs[" + idx +
                                 "]." + field->name_ + ";";
                }
            }

            for (Decl* field : ut->fields_)
            {
                if (field->type_->tag_ != Ptr || !field->attrs_ ||
                    field->attrs_->user_check_)
                    continue;

                std::string prop_val = expr + "[" + idx + "]." + field->name_;
                UserType* field_ut = get_user_type_for_deep_copy(edl_, field);
                if (!field_ut)
                {
                    std::string size = psize(field, expr + "[" + idx + "].");
                    out() << indent + "        " + cmd + "(" + prop_val + ", " +
                                 size + ");";
                }
                else
                {
                    unmarshal_deep_copy(
                        field,
                        expr + "[" + idx + "].",
                        indent + "        ",
                        cmd,
                        level + 1);
                }
            }
            out() << indent + "    }";
        }
        out() << indent + "}";
    }

    void unmarshal_outputs(Function* f)
    {
        std::string check = "OE_CHECK_NULL_TERMINATOR";
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && (p->attrs_->out_ || p->attrs_->inout_))
            {
                empty = false;
                std::string size = psize(p, "_args.");
                std::string cmd = p->attrs_->inout_ ? "OE_READ_IN_OUT_PARAM"
                                                    : "OE_READ_OUT_PARAM";
                if (p->attrs_->string_ || p->attrs_->wstring_)
                {
                    out() << "    " + check +
                                 (p->attrs_->wstring_ ? "_WIDE(" : "(") +
                                 "_output_buffer + _output_buffer_offset, "
                                 "_args." +
                                 p->name_ + "_len);";
                }
                UserType* ut = get_user_type_for_deep_copy(edl_, p);
                if (!ut)
                {
                    out() << "    " + cmd + "(" + p->name_ + ", (size_t)(" +
                                 size + "));";
                    continue;
                }

                unmarshal_deep_copy(p, "", "    ", cmd, 1);
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
