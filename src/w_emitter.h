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
    bool has_deep_copy_out_;

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
                free_fcn = "oe_free";
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
                free_fcn = "oe_free";
                call = "oe_call_enclave_function";
            }
        }
    }

    void emit(Function* f, bool ecall, const std::string& prefix = "")
    {
        ecall_ = ecall;
        has_deep_copy_out_ = has_deep_copy_out(edl_, f);
        std::string alloc_fcn;
        std::string free_fcn;
        std::string call;
        get_functions(f, alloc_fcn, free_fcn, call);
        std::string other = ecall ? "enclave" : "host";
        std::string fcn_id = edl_->name_ + "_fcn_id_" + f->name_;

        std::string args_t = f->name_ + "_args_t";
        /*
         * To avoid duplicated definitions of the ecall wrapper on the host
         * side, the ecall wrapper is defined as [edl_name]_[prefix]_[fun_name].
         * Then we use weak_alias([edl_name]_[prefix]_[fun_name],
         * [prefix]_[fun_name]) to make the exposed [prefix]_[fun_name] weak.
         * Therefore, only one of the implementations will be picked by the
         * linker.
         */
        std::string _prefix = ecall ? edl_->name_ + "_" + prefix : prefix;
        out() << prototype(f, ecall, gen_t(), _prefix) << "{"
              << "    oe_result_t _result = OE_FAILURE;"
              << "";
        if (!gen_t())
        {
            out() << "    static uint64_t global_id = OE_GLOBAL_ECALL_ID_NULL;"
                  << "";
        }
        enclave_status_check();
        out() << "    /* Marshalling struct. */"
              << "    " + args_t +
                     " _args, *_pargs_in = NULL, *_pargs_out = NULL;";
        out() << "    /* Marshalling buffer and sizes. */"
              << "    size_t _input_buffer_size = 0;"
              << "    size_t _output_buffer_size = 0;"
              << "    size_t _total_buffer_size = 0;"
              << "    uint8_t* _buffer = NULL;"
              << "    uint8_t* _input_buffer = NULL;"
              << "    uint8_t* _output_buffer = NULL;"
              << "    size_t _input_buffer_offset = 0;"
              << "    size_t _output_buffer_offset = 0;"
              << "    size_t _output_bytes_written = 0;";
        if (has_deep_copy_out_)
        {
            out() << "    uint8_t* _deepcopy_out_buffer = NULL;"
                  << "    size_t _deepcopy_out_buffer_size = 0;"
                  << "    size_t _deepcopy_out_buffer_offset = 0;";
        }
        out() << ""
              << "    /* Fill marshalling struct. */"
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
        {
            out() << "             enclave,"
                  << "             &global_id,"
                  << "             _" + edl_->name_ + "_ecall_info_table[" +
                         fcn_id + "].name,";
        }
        else
        {
            out() << "             " + fcn_id + ",";
        }
        out() << "             _input_buffer,"
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
              << "    if ((_result = _pargs_out->oe_result) != OE_OK)"
              << "        goto done;"
              << ""
              << "    /* Currently exactly _output_buffer_size bytes must be "
                 "written. */"
              << "    if (_output_bytes_written != _output_buffer_size)"
              << "    {"
              << "        _result = OE_FAILURE;"
              << "        goto done;"
              << "    }"
              << ""
              << "    /* Unmarshal return value and out, in-out parameters. */";
        if (f->rtype_->tag_ != Void)
            out() << "    *_retval = _pargs_out->oe_retval;";
        else
            out() << "    /* No return value. */";
        out() << "";
        if (has_deep_copy_out_)
        {
            out()
                << "    _deepcopy_out_buffer = _pargs_out->deepcopy_out_buffer;"
                << "    _deepcopy_out_buffer_size = "
                   "_pargs_out->deepcopy_out_buffer_size;";
            if (gen_t())
                out() << "    if (_deepcopy_out_buffer && "
                         "_deepcopy_out_buffer_size && "
                      << "        !oe_is_within_enclave(_deepcopy_out_buffer, "
                         "_deepcopy_out_buffer_size))"
                      << "    {"
                      << "        _result = OE_FAILURE;"
                      << "        goto done;"
                      << "    }";
            out() << "";
        }
        unmarshal_outputs(f);
        out() << "";
        if (has_deep_copy_out_)
            out() << "    if (_deepcopy_out_buffer_offset != "
                     "_deepcopy_out_buffer_size)"
                  << "    {"
                  << "        _result = OE_FAILURE;"
                  << "        goto done;"
                  << "    }"
                  << "";
        propagate_errno(f);
        out() << "    _result = OE_OK;"
              << ""
              << "done:"
              << "    if (_buffer)"
              << "        " + free_fcn + "(_buffer);";
        out() << "";
        if (has_deep_copy_out_)
        {
            out() << "    if (_deepcopy_out_buffer)"
                  << "        oe_free(_deepcopy_out_buffer);"
                  << "";
        }
        out() << "    return _result;"
              << "}"
              << "";
        if (!gen_t())
            out() << "OE_WEAK_ALIAS(" + _prefix + f->name_ + ", " + prefix +
                         f->name_ + ");"
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
            std::string prefix = "_args." + parent_expr + op;
            std::string argcount = pcount(prop, prefix);
            std::string argsize = psize(prop, prefix);
            std::string cond = parent_condition + " && " + expr;
            out() << indent + "if (" + cond + ")"
                  << indent + "    OE_ADD_ARG_SIZE(" + buffer_size + ", " +
                         argcount + ", " + argsize + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count = count_attr_str(prop->attrs_->count_, prefix);

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

            std::string argcount = pcount(p, "_args.");
            std::string argsize = psize(p, "_args.");
            out() << "    if (" + p->name_ + ")"
                  << "        OE_ADD_ARG_SIZE(" + buffer_size + ", " +
                         argcount + ", " + argsize + ");";
            empty = false;

            /* Skip the nested pointers if the parameter is not
             * deep-copyable or has the out-only attribute. */
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut || (p->attrs_->out_ && !p->attrs_->inout_))
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
            std::string prefix = "_args." + parent_expr + op;
            std::string argcount = pcount(prop, prefix);
            std::string argsize = psize(prop, prefix);
            std::string cond = parent_condition + " && " + expr;
            std::string mt = mtype_str(prop);
            out() << indent + "if (" + cond + ")"
                  << indent + "    " + cmd + "(" + expr + ", " + argcount +
                         ", " + argsize + ", " + mt + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count = count_attr_str(prop->attrs_->count_, prefix);

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
                std::string argcount = pcount(p, "_args.");
                std::string argsize = psize(p, "_args.");
                std::string cmd = p->attrs_->inout_ ? "OE_WRITE_IN_OUT_PARAM"
                                                    : "OE_WRITE_IN_PARAM";
                out() << "    if (" + p->name_ + ")"
                      << "        " + cmd + "(" + p->name_ + ", " + argcount +
                             ", " + argsize + ", " + mt + ");";
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
            return;

        /*
         * Deep copied structure. Unmarshal individual fields.
         */
        out() << indent + "if (" + expr + ")" << indent + "{";
        {
            std::string argcount = pcount(p, parent_expr);
            std::string argsize = psize(p, parent_expr);
            std::string p_type = atype_str(p->type_);
            out()
                << indent + "    " + p_type + " _rhs = (" + p_type +
                       ") (_output_buffer + _output_buffer_offset); (void)_rhs;"
                << indent + "    OE_ADD_ARG_SIZE(_output_buffer_offset, " +
                       argcount + ", " + argsize + ");";

            std::string idx = "_i_" + to_str(level);

            out() << indent + "    for (size_t " + idx + " = 0; " + idx +
                         " < " + argcount + "; " + idx + "++)"
                  << indent + "    {";

            /* First iteration: Find struct members that are not user-defined
             * pointers. */
            for (Decl* field : ut->fields_)
            {
                std::string lhs_val = expr + "[" + idx + "]." + field->name_;
                std::string rhs_val = "_rhs[" + idx + "]." + field->name_;
                /*
                 * We currently do not update the struct member based on the
                 * value set by the callee if the member is used by the size or
                 * count attribute. However, we still report an error if the
                 * callee sets a value that is larger than the supplied one.
                 * This warns the caller that the callee behaves unexpectedly.
                 */
                if (field->attrs_ && field->attrs_->is_size_or_count_)
                {
                    out() << indent + "        " + "if (" + lhs_val + " < " +
                                 rhs_val + ")"
                          << indent + "        {"
                          << indent + "            _result = OE_FAILURE;"
                          << indent + "            goto done;"
                          << indent + "        }";
                    continue;
                }

                if (field->type_->tag_ != Ptr || !field->attrs_ ||
                    field->attrs_->user_check_)
                {
                    out() << indent + "        " + lhs_val + " = " + rhs_val +
                                 ";";
                }
            }

            /* Second iteration: Find struct members that are user-defined
             * pointers. */
            for (Decl* field : ut->fields_)
            {
                if (field->type_->tag_ != Ptr || !field->attrs_ ||
                    field->attrs_->user_check_ ||
                    field->attrs_->is_size_or_count_)
                    continue;

                std::string prop_val = expr + "[" + idx + "]." + field->name_;
                UserType* field_ut = get_user_type_for_deep_copy(edl_, field);
                if (!field_ut)
                {
                    std::string argcount =
                        pcount(field, expr + "[" + idx + "].");
                    std::string argsize = psize(field, expr + "[" + idx + "].");
                    out() << indent + "        " + cmd + "(" + prop_val + ", " +
                                 argcount + ", " + argsize + ");";
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

    void unserialize_pointers_deep_copy(
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
            std::string argcount = pcount(prop, parent_expr + op);
            std::string argsize = psize(prop, parent_expr + op);
            std::string cond = parent_condition + " && " + expr;
            std::string mt = mtype_str(prop);
            out() << indent + "if (" + cond + ")"
                  << indent + "    " + cmd + "(" + expr + ", " + argcount +
                         ", " + argsize + ", " + mt + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count =
                count_attr_str(prop->attrs_->count_, parent_expr + op);

            if (count == "1" || count == "")
            {
                unserialize_pointers_deep_copy(
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
                std::string cond =
                    parent_condition + " && " + parent_expr + op + prop->name_;
                unserialize_pointers_deep_copy(
                    cond, expr, cmd, prop, level + 1, indent + "    ");
                out() << indent + "}";
            }
        });
    }

    void unmarshal_deep_copy_out(Decl* p)
    {
        std::string cmd = "OE_SET_DEEPCOPY_OUT_PARAM";
        std::string count = count_attr_str(p->attrs_->count_);
        std::string mt = mtype_str(p);

        if (count == "1" || count == "")
        {
            std::string cond = p->name_;
            std::string expr = p->name_;
            unserialize_pointers_deep_copy(cond, expr, cmd, p, 2, "    ");
        }
        else
        {
            std::string cond = p->name_;
            std::string expr = p->name_ + "[_i_1]";
            out() << "    for (size_t _i_1 = 0; _i_1 < " + count + "; _i_1++)"
                  << "    {";
            unserialize_pointers_deep_copy(cond, expr, cmd, p, 2, "        ");
            out() << "    }";
        }
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
                std::string argcount = pcount(p, "_args.");
                std::string argsize = psize(p, "_args.");
                std::string cmd = p->attrs_->inout_ ? "OE_READ_IN_OUT_PARAM"
                                                    : "OE_READ_OUT_PARAM";
                UserType* ut = get_user_type_for_deep_copy(edl_, p);
                if (!ut)
                {
                    out() << "    " + cmd + "(" + p->name_ + ", " + argcount +
                                 ", " + argsize + ");";
                }
                if (p->attrs_->string_ || p->attrs_->wstring_)
                {
                    out() << "    " + check +
                                 (p->attrs_->wstring_ ? "_WIDE(" : "(") +
                                 p->name_ + ", _args." + p->name_ + "_len);";
                }

                if (ut)
                {
                    if (p->attrs_->inout_)
                        unmarshal_deep_copy(p, "", "    ", cmd, 1);
                    else
                    {
                        out() << "    OE_READ_OUT_PARAM(" + p->name_ + ", " +
                                     argcount + ", " + argsize + ");";
                        unmarshal_deep_copy_out(p);
                    }
                }
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
            out() << "    oe_errno = _pargs_out->ocall_errno;"
                  << "";
        else
            out() << "    /* Errno propagation not enabled. */";
        out() << "";
    }
};

#endif // W_EMITTER_H
