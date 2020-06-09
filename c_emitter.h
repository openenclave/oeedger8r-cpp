// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef C_EMITTER_H
#define C_EMITTER_H

#include <fstream>

#include "ast.h"
#include "f_emitter.h"
#include "utils.h"
#include "w_emitter.h"

class CEmitter
{
    Edl* edl_;
    bool gen_t_c_;
    std::ofstream file_;
    std::string indent_;

  public:
    typedef CEmitter& R;
    R out()
    {
        return *this;
    }

    template <typename T>
    R operator<<(const T& t)
    {
        file_ << indent_ << t << "\n";
        return out();
    }
    template <typename T>
    R operator<<(const T* t)
    {
        if (t)
            file_ << indent_ << t << "\n";
        return out();
    }

  public:
    CEmitter(Edl* edl) : edl_(edl), gen_t_c_(false), file_(), indent_()
    {
    }

    void emit_t_c(const std::string& dir_with_sep = "")
    {
        gen_t_c_ = true;
        file_.open(dir_with_sep + edl_->name_ + "_t.c");
        autogen_preamble(out());
        out() << "#include \"" + edl_->name_ + "_t.h\""
              << ""
              << "#include <openenclave/edger8r/" +
                     std::string(gen_t_c_ ? "enclave.h>" : "host.h>")
              << ""
              << "OE_EXTERNC_BEGIN"
              << ""
              << "/**** Trusted function IDs ****/";
        trusted_function_ids();
        out() << "/**** ECALL marshalling structs. ****/";
        ecall_marshalling_structs();
        out() << "/**** ECALL functions. ****/"
              << "";
        for (Function* f : edl_->trusted_funcs_)
            emit_forwarder(f);
        out() << "/**** ECALL function table. ****/"
              << "";
        ecalls_table();
        out() << "/**** Untrusted function IDs. ****/";
        untrusted_function_ids();
        out() << "/**** OCALL marshalling structs. ****/";
        ocall_marshalling_structs();
        out() << "/**** OCALL function wrappers. ****/"
              << "";
        for (Function* f : edl_->untrusted_funcs_)
            emit_wrapper(f);
        if (edl_->untrusted_funcs_.empty())
            out() << "/* There were no ocalls. */";
        out() << "OE_EXTERNC_END";
        file_.close();
    }

    void emit_u_c(
        const std::string& dir_with_sep = "",
        const std::string& prefix = "")
    {
        gen_t_c_ = false;
        file_.open(dir_with_sep + edl_->name_ + "_u.c");
        autogen_preamble(out());
        out() << "#include \"" + edl_->name_ + "_u.h\""
              << ""
              << "#include <openenclave/edger8r/host.h>"
              << ""
              << "OE_EXTERNC_BEGIN"
              << ""
              << "/**** Trusted function IDs ****/";
        trusted_function_ids();
        out() << "/**** ECALL marshalling structs. ****/";
        ecall_marshalling_structs();
        out() << "/**** ECALL function wrappers. ****/"
              << "";
        for (Function* f : edl_->trusted_funcs_)
            emit_wrapper(f, prefix);
        out() << "/**** Untrusted function IDs. ****/";
        untrusted_function_ids();
        out() << "/**** OCALL marshalling structs. ****/";
        ocall_marshalling_structs();
        out() << "/**** OCALL functions. ****/"
              << "";
        for (Function* f : edl_->untrusted_funcs_)
            emit_forwarder(f);
        if (edl_->untrusted_funcs_.empty())
            out() << "/* There were no ocalls. */"
                  << "";
        out() << "/**** OCALL function table. ****/"
              << "";
        ocalls_table();
        out() << create_prototype(edl_->name_) << "{"
              << "    return oe_create_enclave("
              << "               path,"
              << "               type,"
              << "               flags,"
              << "               settings,"
              << "               setting_count,"
              << "               __" + edl_->name_ + "_ocall_function_table,"
              << "               " + to_str(edl_->untrusted_funcs_.size()) + ","
              << "               enclave);"
              << "}"
              << ""
              << "OE_EXTERNC_END";
        file_.close();
    }

    void trusted_function_ids()
    {
        out() << "enum"
              << "{";
        int idx = 0;
        std::string pfx = "    " + edl_->name_ + "_fcn_id_";
        for (Function* f : edl_->trusted_funcs_)
            out() << pfx + f->name_ + " = " + to_str(idx++) + ",";
        out() << pfx + "trusted_call_id_max = OE_ENUM_MAX"
              << "};"
              << "";
    }

    void untrusted_function_ids()
    {
        out() << "enum"
              << "{";
        int idx = 0;
        std::string pfx = "    " + edl_->name_ + "_fcn_id_";
        for (Function* f : edl_->untrusted_funcs_)
            out() << pfx + f->name_ + " = " + to_str(idx++) + ",";
        out() << pfx + "untrusted_call_max = OE_ENUM_MAX"
              << "};"
              << "";
    }
    void ecall_marshalling_structs()
    {
        for (Function* f : edl_->trusted_funcs_)
            marshalling_struct(f, false);
    }

    void ocall_marshalling_structs()
    {
        for (Function* f : edl_->untrusted_funcs_)
            marshalling_struct(f, true);
    }

    void marshalling_struct(Function* f, bool ocall = false)
    {
        (void)ocall;
        out() << "typedef struct _" + f->name_ + "_args_t"
              << "{"
              << "    oe_result_t _result;";
        indent_ = "    ";
        if (f->rtype_->tag_ != Void)
            out() << atype_str(f->rtype_) + " _retval;";
        for (Decl* p : f->params_)
        {
            out() << mdecl_str(p->name_, p->type_, p->dims_, p->attrs_) + ";";
            if (p->attrs_ && (p->attrs_->string_ || p->attrs_->wstring_))
                out() << "size_t " + p->name_ + "_len;";
        }
        if (f->errno_)
            out() << "int _ocall_errno;";
        indent_ = "";
        out() << "} " + f->name_ + "_args_t;"
              << "";
    }

    void ecalls_table()
    {
        out() << "oe_ecall_func_t __oe_ecalls_table[] = {";
        size_t idx = 0;
        for (Function* f : edl_->trusted_funcs_)
            out() << "    (oe_ecall_func_t) ecall_" + f->name_ +
                         (++idx < edl_->trusted_funcs_.size() ? "," : "");
        out() << "};"
              << ""
              << "size_t __oe_ecalls_table_size = "
                 "OE_COUNTOF(__oe_ecalls_table);"
              << "";
    }

    void ocalls_table()
    {
        out() << "static oe_ocall_func_t __" + edl_->name_ +
                     "_ocall_function_table[] = {";
        for (Function* f : edl_->untrusted_funcs_)
            out() << "    (oe_ocall_func_t) ocall_" + f->name_ + ",";
        out() << "    NULL"
              << "};"
              << "";
    }

    void emit_forwarder(Function* f)
    {
        FEmitter(edl_, file_).emit(f, gen_t_c_);
    }

    void emit_wrapper(Function* f, const std::string& prefix = "")
    {
        WEmitter(edl_, file_).emit(f, !gen_t_c_, prefix);
    }
};

#endif // C_EMITTER_H
