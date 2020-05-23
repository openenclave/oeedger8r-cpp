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
              << "    " + args_t + "* pargs_in = (" + args_t + "*)input_buffer;"
              << "    " + args_t + "* pargs_out = (" + args_t +
                     "*)output_buffer;"
              << ""
              << "    size_t input_buffer_offset = 0;"
              << "    size_t output_buffer_offset = 0;"
              << "    OE_ADD_SIZE(input_buffer_offset, sizeof(*pargs_in));"
              << "    OE_ADD_SIZE(output_buffer_offset, sizeof(*pargs_out));"
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
        if (ecall)
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
        propagate_errno(f);
        out() << "    /* Success. */"
              << "    _result = OE_OK;"
              << "    *output_bytes_written = output_buffer_offset;"
              << ""
              << "done:";
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
            std::string size = psize(prop, "pargs_in->" + parent_expr + op);
            std::string cond = parent_condition + " && " + (is_out ? "!" : "") +
                               "pargs_in->" + expr;
            std::string mt = mtype_str(prop);
            out() << indent + "if (" + cond + ")"
                  << indent + "    " + cmd + "(" + expr + ", " + size + ", " +
                         mt + ");";

            UserType* ut = get_user_type_for_deep_copy(edl_, prop);
            if (!ut)
                return;

            std::string count =
                count_attr_str(prop->attrs_->count_, "pargs_in->");

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
                std::string cond = parent_condition + " && pargs_in->" +
                                   parent_expr + op + prop->name_;
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
            std::string size = psize(p, "pargs_in->");
            std::string cmd = (p->attrs_->inout_) ? "OE_SET_IN_OUT_POINTER"
                                                  : "OE_SET_IN_POINTER";
            out() << "    if (pargs_in->" + p->name_ + ")"
                  << "        " + cmd + "(" + p->name_ + ", " + size + ", " +
                         mtype_str(p) + ");";

            empty = false;
            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut)
                continue;

            std::string count = count_attr_str(p->attrs_->count_, "pargs_in->");

            if (count == "1" || count == "")
            {
                std::string cond = "pargs_in->" + p->name_;
                std::string expr = p->name_;
                set_pointers_deep_copy(cond, expr, cmd, p, 2, "    ");
            }
            else
            {
                std::string cond = "pargs_in->" + p->name_;
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
            std::string size = psize(p, "pargs_in->");
            std::string cmd = (p->attrs_->inout_)
                                  ? "OE_COPY_AND_SET_IN_OUT_POINTER"
                                  : "OE_SET_OUT_POINTER";
            out() << "    if (pargs_in->" + p->name_ + ")"
                  << "        " + cmd + "(" + p->name_ + ", " + size + ", " +
                         mtype_str(p) + ");";
            empty = false;

            UserType* ut = get_user_type_for_deep_copy(edl_, p);
            if (!ut)
                continue;

            std::string count = count_attr_str(p->attrs_->count_, "pargs_in->");

            if (count == "1" || count == "")
            {
                std::string cond = "pargs_in->" + p->name_;
                std::string expr = p->name_;
                set_pointers_deep_copy(
                    cond, expr, cmd, p, 2, "    ", p->attrs_->out_);
            }
            else
            {
                std::string cond = "pargs_in->" + p->name_;
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

    void check_null_terminators(Function* f)
    {
        std::string check = "OE_CHECK_NULL_TERMINATOR";
        bool strs = false;
        for (Decl* p : f->params_)
        {
            if (!p->attrs_ || !(p->attrs_->string_ || p->attrs_->wstring_))
                continue;
            out() << "    " + check + (p->attrs_->wstring_ ? "_WIDE" : "") +
                         "(pargs_in->" + p->name_ + ", pargs_in->" + p->name_ +
                         "_len);";
            strs = true;
        }
        if (!strs)
            out() << "    /* There were no in nor in-out string parameters. */";
        out() << "";
    }

    void call_user_function(Function* f)
    {
        std::string retstr =
            (f->rtype_->tag_ != Void) ? "pargs_out->_retval = " : "";
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
            out() << "        " + type + "pargs_in->" + p->name_ +
                         (++idx < f->params_.size() ? "," : ");");
        }
        if (idx == 0)
            out() << "    );";
        out() << "";
    }

    void propagate_errno(Function* f)
    {
        if (ecall_)
            return;
        out() << "    /* Propagate errno back to enclave. */";
        if (f->errno_)
            out() << "    pargs_out->_ocall_errno = errno;";
        else
            out() << "    /* Errno propagation not enabled. */";
        out() << "";
    }

    void write_result()
    {
        std::string check = "output_buffer_size >= sizeof(*pargs_out)";
        if (ecall_)
            out() << "    if (" + check +
                         " &&\n        oe_is_within_enclave(pargs_out, "
                         "output_buffer_size))";
        else
            out() << "    if (pargs_out && " + check + ")";
        out() << "        pargs_out->_result = _result;";
    }
};

#endif // F_EMITTER_H
