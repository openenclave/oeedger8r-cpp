// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef F_EMITTER_H
#define F_EMITTER_H

#include <fstream>

#include "ast.h"
#include "utils.h"

class FEmitter
{
    Edl* edl_;
    std::ofstream& file_;
    bool ecall_;
    bool has_deep_copy_out_;

  public:
    typedef FEmitter& R;
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
    FEmitter(Edl* edl, std::ofstream& file)
        : edl_(edl), file_(file), ecall_(true)
    {
        (void)edl_;
    }

    void emit(Function* f, bool ecall)
    {
        ecall_ = ecall;
        has_deep_copy_out_ = has_deep_copy_out(edl_, f);
        std::string pfx = ecall_ ? "ecall_" : "ocall_";
        std::string args_t = f->name_ + "_args_t";
        out() << "static void " + pfx + f->name_ + "("
              << "    uint8_t* input_buffer,"
              << "    size_t input_buffer_size,"
              << "    uint8_t* output_buffer,"
              << "    size_t output_buffer_size,"
              << "    size_t* output_bytes_written)"
              << "{"
              << "    oe_result_t _result = OE_FAILURE;";
        if (!ecall_)
            out() << "    OE_UNUSED(input_buffer_size);";
        out() << ""
              << "    /* Prepare parameters. */"
              << "    " + args_t + "* _pargs_in = (" + args_t +
                     "*)input_buffer;"
              << "    " + args_t + "* _pargs_out = (" + args_t +
                     "*)output_buffer;"
              << "";
        if (has_deep_copy_out_)
        {
            out() << "    uint8_t* _deepcopy_out_buffer = NULL;"
                  << "    size_t _deepcopy_out_buffer_offset = 0;"
                  << "    size_t _deepcopy_out_buffer_size = 0;"
                  << "";
        }
        out() << "    size_t _input_buffer_offset = 0;"
              << "    size_t _output_buffer_offset = 0;"
              << "    OE_ADD_SIZE(_input_buffer_offset, sizeof(*_pargs_in));"
              << "    OE_ADD_SIZE(_output_buffer_offset, sizeof(*_pargs_out));"
              << ""
              << "    if (input_buffer_size < sizeof(*_pargs_in) || "
                 "output_buffer_size < sizeof(*_pargs_in))"
              << "        goto done;"
              << "";
        if (ecall_)
            ecall_buffer_checks();
        else
            ocall_buffer_checks();
        out() << "    /* Set in and in-out pointers. */";
        set_in_in_out_pointers(f);
        out() << "    /* Set out and in-out pointers. */"
              << "    /* In-out parameters are copied to output buffer. */";
        set_out_in_out_pointers(f);
        if (ecall_)
        {
            out() << "    /* Check that in/in-out strings are null terminated. "
                     "*/";
            check_null_terminators(f);
            out() << "    /* lfence after checks. */"
                  << "    oe_lfence();"
                  << "";
        }
        out() << "    /* Call user function. */";
        call_user_function(f);
        if (has_deep_copy_out_)
        {
            out() << "    /* Compute the size for the deep-copy out buffer. */";
            compute_buffer_size_deep_copy_out(f);
            out() << ""
                  << "    if (_deepcopy_out_buffer_size)"
                  << "    {"
                  << "        _deepcopy_out_buffer = (uint8_t*) "
                     "oe_malloc(_deepcopy_out_buffer_size);"
                  << "        if (!_deepcopy_out_buffer)"
                  << "        {"
                  << "            _result = OE_OUT_OF_MEMORY;"
                  << "            goto done;"
                  << "        }"
                  << "    }"
                  << "";
            out() << "    /* Serialize the deep-copied content into the "
                     "buffer. */";
            serialize_buffer_deep_copy_out(f);
            out()
                << "    if (_deepcopy_out_buffer_offset != "
                   "_deepcopy_out_buffer_size)"
                << "    {"
                << "        _result = OE_FAILURE;"
                << "        goto done;"
                << "    }"
                << ""
                << "    /* Set the _deepcopy_out_buffer and "
                   "_deepcopy_out_buffer as part of _pargs_out. */"
                << "    _pargs_out->deepcopy_out_buffer = _deepcopy_out_buffer;"
                << "    _pargs_out->deepcopy_out_buffer_size = "
                   "_deepcopy_out_buffer_size;"
                << "";
        }
        else
            out() << "    /* There is no deep-copyable out parameter. */"
                  << "    _pargs_out->deepcopy_out_buffer = NULL;"
                  << "    _pargs_out->deepcopy_out_buffer_size = 0;"
                  << "";
        propagate_errno(f);
        out() << "    /* Success. */"
              << "    _result = OE_OK;"
              << "    *output_bytes_written = _output_buffer_offset;"
              << ""
              << "done:";
        if (has_deep_copy_out_)
        {
            out()
                << "    /* Free _pargs_out->deepcopy_out_buffer on failure. */"
                << "    if (_result != OE_OK)"
                << "    {"
                << "        oe_free(_pargs_out->deepcopy_out_buffer);"
                << "        _pargs_out->deepcopy_out_buffer = NULL;"
                << "        _pargs_out->deepcopy_out_buffer_size = 0;"
                << "    }"
                << "";
            out() << "    /* Free nested buffers allocated by the user "
                     "function. */";
            free_deep_copy_out(f, "_pargs_in->", "_pargs_in->", "    ");
            out() << "";
        }
        write_result();
        out() << "}"
              << "";
    }

    void ecall_buffer_checks()
    {
        out() << "    /* Make sure input and output buffers lie within the "
                 "enclave. */"
              << "    /* oe_is_within_enclave explicitly checks if buffers "
                 "are null or not. */"
              << "    if (!oe_is_within_enclave(input_buffer, "
                 "input_buffer_size))"
              << "        goto done;"
              << ""
              << "    if (!oe_is_within_enclave(output_buffer, "
                 "output_buffer_size))"
              << "        goto done;"
              << "";
    }

    void ocall_buffer_checks()
    {
        out() << "    /* Make sure input and output buffers are valid. */"
              << "    if (!input_buffer || !output_buffer) {"
              << "        _result = OE_INVALID_PARAMETER;"
              << "        goto done;"
              << "    }"
              << "";
    }

    void set_pointers_deep_copy(
        const std::string& parent_condition,
        const std::string& parent_expr,
        const std::string& cmd,
        Decl* parent_prop,
        int level,
        std::string indent = "    ",
        bool is_out = false)
    {
        UserType* ut = get_user_type_for_deep_copy(edl_, parent_prop);
        if (!ut)
            return;
        iterate_deep_copyable_fields(ut, [&](Decl* prop) {
            std::string op = *parent_expr.rbegin() == ']' ? "." : "->";
            std::string expr = parent_expr + op + prop->name_;
            std::string prefix = "_pargs_in->" + parent_expr + op;
            std::string argcount = pcount(prop, prefix);
            std::string argsize = psize(prop, prefix);
            std::string cond = parent_condition + " && " + (is_out ? "!" : "") +
                               "_pargs_in->" + expr;
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
                set_pointers_deep_copy(
                    cond, expr, cmd, prop, level + 1, indent, is_out);
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
                    parent_condition + " && " + prefix + prop->name_;
                set_pointers_deep_copy(
                    cond, expr, cmd, prop, level + 1, indent + "    ", is_out);
                out() << indent + "}";
            }
        });
    }

    void set_in_in_out_pointers(Function* f)
    {
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (!p->attrs_ || !(p->attrs_->in_ || p->attrs_->inout_))
                continue;
            std::string argcount = pcount(p, "_pargs_in->");
            std::string argsize = psize(p, "_pargs_in->");
            std::string cmd = (p->attrs_->inout_) ? "OE_SET_IN_OUT_POINTER"
                                                  : "OE_SET_IN_POINTER";
            out() << "    if (_pargs_in->" + p->name_ + ")"
                  << "        " + cmd + "(" + p->name_ + ", " + argcount +
                         ", " + argsize + ", " + mtype_str(p) + ");";

            empty = false;
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut)
                continue;

            std::string count =
                count_attr_str(p->attrs_->count_, "_pargs_in->");

            if (count == "1" || count == "")
            {
                std::string cond = "_pargs_in->" + p->name_;
                std::string expr = p->name_;
                set_pointers_deep_copy(cond, expr, cmd, p, 2, "    ");
            }
            else
            {
                std::string cond = "_pargs_in->" + p->name_;
                std::string expr = p->name_ + "[_i_1]";
                out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                             "; _i_1++)"
                      << "    {";
                set_pointers_deep_copy(cond, expr, cmd, p, 2, "        ");
                out() << "    }";
            }
        }
        if (empty)
            out() << "    /* There were no in nor in-out parameters. */";
        out() << "";
    }

    void set_out_in_out_pointers(Function* f)
    {
        bool empty = true;
        for (Decl* p : f->params_)
        {
            if (!p->attrs_ || !(p->attrs_->out_ || p->attrs_->inout_))
                continue;

            std::string argcount = pcount(p, "_pargs_in->");
            std::string argsize = psize(p, "_pargs_in->");
            std::string cmd = (p->attrs_->inout_)
                                  ? "OE_COPY_AND_SET_IN_OUT_POINTER"
                                  : "OE_SET_OUT_POINTER";
            out() << "    if (_pargs_in->" + p->name_ + ")"
                  << "        " + cmd + "(" + p->name_ + ", " + argcount +
                         ", " + argsize + ", " + mtype_str(p) + ");";
            empty = false;

            /* Skip on setting nested pointers if the parameter is not
             * deep-copyable or has the out-only attribute. */
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut || (p->attrs_->out_ && !p->attrs_->inout_))
                continue;

            std::string count =
                count_attr_str(p->attrs_->count_, "_pargs_in->");

            if (count == "1" || count == "")
            {
                std::string cond = "_pargs_in->" + p->name_;
                std::string expr = p->name_;
                set_pointers_deep_copy(
                    cond, expr, cmd, p, 2, "    ", p->attrs_->out_);
            }
            else
            {
                std::string cond = "_pargs_in->" + p->name_;
                std::string expr = p->name_ + "[_i_1]";
                out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                             "; _i_1++)"
                      << "    {";
                set_pointers_deep_copy(
                    cond, expr, cmd, p, 2, "        ", p->attrs_->out_);
                out() << "    }";
            }
        }
        if (empty)
            out() << "    /* There were no out nor in-out parameters. */";
        out() << "";
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
            std::string argcount = pcount(prop, parent_expr + op);
            std::string argsize = psize(prop, parent_expr + op);
            std::string cond = parent_condition + " && " + expr;
            out() << indent + "if (" + cond + ")"
                  << indent + "    OE_ADD_ARG_SIZE(" + buffer_size + ", " +
                         argcount + ", " + argsize + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count =
                count_attr_str(prop->attrs_->count_, parent_expr + op);

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

    void compute_buffer_size_deep_copy_out(Function* f)
    {
        std::string buffer_size = "_deepcopy_out_buffer_size";
        std::string prefix = "_pargs_in->";
        for (Decl* p : f->params_)
        {
            if (!p->attrs_)
                continue;

            /* Skip the nested pointers if the parameter is neither
             * deep-copyable nor has the out-only attribute. */
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut || !p->attrs_->out_ || p->attrs_->inout_)
                continue;

            std::string count = count_attr_str(p->attrs_->count_, prefix);

            if (count == "1" || count == "")
            {
                std::string cond = prefix + p->name_;
                std::string expr = prefix + p->name_;
                add_size_deep_copy(cond, expr, buffer_size, p, 2, "    ");
            }
            else
            {
                std::string cond = prefix + p->name_;
                std::string expr = prefix + p->name_ + "[_i_1]";
                out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                             "; _i_1++)"
                      << "    {";
                add_size_deep_copy(cond, expr, buffer_size, p, 2, "        ");
                out() << "    }";
            }
        }
    }

    void check_null_terminators(Function* f)
    {
        std::string check = "OE_CHECK_NULL_TERMINATOR";
        bool strs = false;
        for (Decl* p : f->params_)
        {
            if (!p->attrs_ || !(p->attrs_->string_ || p->attrs_->wstring_))
                continue;
            out() << "    " + check + (p->attrs_->wstring_ ? "_WIDE" : "") +
                         "(_pargs_in->" + p->name_ + ", _pargs_in->" +
                         p->name_ + "_len);";
            strs = true;
        }
        if (!strs)
            out() << "    /* There were no in nor in-out string parameters. */";
        out() << "";
    }

    void call_user_function(Function* f)
    {
        std::string retstr =
            (f->rtype_->tag_ != Void) ? "_pargs_out->oe_retval = " : "";
        out() << "    " + retstr + f->name_ + "(";
        size_t idx = 0;
        for (Decl* p : f->params_)
        {
            std::string type = "";
            if (p->dims_ && !p->dims_->empty())
            {
                type = decl_str("(*)", p->type_, p->dims_);
                type = "*(" + type + ")";
            }
            else if (
                p->type_->tag_ == Foreign && p->attrs_ && p->attrs_->isary_)
            {
                type = "/* foreign array */ *(" + p->type_->name_ + "*)";
            }
            else if (p->type_->tag_ == Ptr)
            {
                std::string s = atype_str(p->type_);
                if (s.find("const ") != std::string::npos)
                    type = "(" + s + ")";
            }
            out() << "        " + type + "_pargs_in->" + p->name_ +
                         (++idx < f->params_.size() ? "," : ");");
        }
        if (idx == 0)
            out() << "    );";
        out() << "";
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
            std::string argcount = pcount(prop, parent_expr + op);
            std::string argsize = psize(prop, parent_expr + op);
            std::string cond = parent_condition + " && " + expr;
            out() << indent + "if (" + cond + ")"
                  << indent + "    " + cmd + "(" + expr + ", " + argcount +
                         ", " + argsize + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count =
                count_attr_str(prop->attrs_->count_, parent_expr + op);

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

    void serialize_buffer_deep_copy_out(Function* f)
    {
        std::string prefix = "_pargs_in->";
        for (Decl* p : f->params_)
        {
            if (!p->attrs_)
                continue;

            /* Skip the nested pointers if the parameter is neither
             * deep-copyable nor has the out-only attribute. */
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut || !p->attrs_->out_ || p->attrs_->inout_)
                continue;

            std::string count = count_attr_str(p->attrs_->count_, prefix);
            std::string cmd = "OE_WRITE_DEEPCOPY_OUT_PARAM";
            std::string mt = mtype_str(p);

            if (count == "1" || count == "")
            {
                std::string cond = prefix + p->name_;
                std::string expr = prefix + p->name_;
                serialize_pointers_deep_copy(cond, expr, cmd, p, 2, "    ");
            }
            else
            {
                std::string cond = prefix + p->name_;
                std::string expr = prefix + p->name_ + "[_i_1]";
                out() << "    for (size_t _i_1 = 0; _i_1 < " + count +
                             "; _i_1++)"
                      << "    {";
                serialize_pointers_deep_copy(cond, expr, cmd, p, 2, "        ");
                out() << "    }";
            }
        }
        out() << "";
    }

    void free_pointers_deep_copy(
        Decl* p,
        std::string parent_lhs_expr,
        std::string parent_rhs_expr,
        const std::string& indent,
        int level = 1)
    {
        std::string cmd = "free";
        std::string lhs_expr = parent_lhs_expr + p->name_;
        std::string rhs_expr = parent_rhs_expr + p->name_;
        UserType* ut = get_user_type_for_deep_copy(edl_, p);
        if (!ut)
            return;

        /*
         * Deep copied structure. Unmarshal individual fields.
         */
        out() << indent + "if (" + lhs_expr + ")" << indent + "{";
        {
            std::string count = pcount(p, parent_rhs_expr);
            std::string idx = "_i_" + to_str(level);

            out() << indent + "    for (size_t " + idx + " = 0; " + idx +
                         " < " + count + "; " + idx + "++)"
                  << indent + "    {";

            for (Decl* field : ut->fields_)
            {
                if (field->type_->tag_ != Ptr || !field->attrs_ ||
                    field->attrs_->user_check_ ||
                    field->attrs_->is_size_or_count_)
                    continue;

                std::string lhs_val =
                    lhs_expr + "[" + idx + "]." + field->name_;
                std::string rhs_val =
                    rhs_expr + "[" + idx + "]." + field->name_;
                std::string ptr_prefix = "_l_" + std::to_string(level) + "_";
                UserType* field_ut = get_user_type_for_deep_copy(edl_, field);
                if (field_ut)
                {
                    /* Free the nested pointers first. */
                    free_pointers_deep_copy(
                        field,
                        lhs_expr + "[" + idx + "].",
                        rhs_expr + "[" + idx + "].",
                        indent + "        ",
                        level + 1);
                }
                out() << indent + "        " + cmd + "(" + lhs_val + ");";
            }
            out() << indent + "    }";
        }
        out() << indent + "}";
    }

    void free_deep_copy_out(
        Function* f,
        std::string lhs_prefix,
        std::string rhs_prefix,
        const std::string& indent)
    {
        for (Decl* p : f->params_)
        {
            if (p->attrs_ && p->attrs_->out_ && !p->attrs_->inout_)
            {
                UserType* ut = get_user_type_for_deep_copy(edl_, p);
                if (!ut)
                    continue;
                free_pointers_deep_copy(p, lhs_prefix, rhs_prefix, indent, 1);
            }
        }
    }

    void propagate_errno(Function* f)
    {
        if (ecall_)
            return;
        out() << "    /* Propagate errno back to enclave. */";
        if (f->errno_)
            out() << "    _pargs_out->ocall_errno = errno;";
        else
            out() << "    /* Errno propagation not enabled. */";
        out() << "";
    }

    void write_result()
    {
        std::string check = "output_buffer_size >= sizeof(*_pargs_out)";
        if (ecall_)
            out() << "    if (" + check +
                         " &&\n        oe_is_within_enclave(_pargs_out, "
                         "output_buffer_size))";
        else
            out() << "    if (_pargs_out && " + check + ")";
        out() << "        _pargs_out->oe_result = _result;";
    }
};

#endif // F_EMITTER_H
