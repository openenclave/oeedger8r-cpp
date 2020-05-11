// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef H_EMITTER_H
#define H_EMITTER_H

#include <fstream>
#include "ast.h"

class HEmitter
{
    Edl* edl_;
    bool gen_t_h_;
    std::ofstream file_;
    std::string indent_;

  public:
    typedef HEmitter& R;
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
    HEmitter(Edl* edl) : edl_(edl), gen_t_h_(false), file_(), indent_()
    {
    }

    void emit_t_h()
    {
        gen_t_h_ = true;
        file_.open(edl_->name_ + "_t_new.h");
        indent_ = "";
        emit_h();
        file_.close();
    }

    void emit_u_h()
    {
        gen_t_h_ = false;
        file_.open(edl_->name_ + "_u_new.h");
        emit_h();
        file_.close();
    }

    void emit_h()
    {
        std::string guard =
            "EDGER8R_" + upper(edl_->name_) + (gen_t_h_ ? "_T" : "_U") + "_H";
        std::string inc = (gen_t_h_ ? "enclave" : "host");
        header(out(), guard);
        out() << ""
              << "#include <openenclave/" + inc + ".h>"
              << ""
              << "#include \"" + edl_->name_ + "_args.h\""
              << ""
              << "OE_EXTERNC_BEGIN"
              << "";
        if (!gen_t_h_)
            out() << create_prototype(edl_->name_) + ";"
                  << "";
        out() << "/**** Trusted function IDs ****/";
        trusted_function_ids();
        out() << "/**** ECALL marshalling structs. ****/";
        ecall_marshalling_structs();
        out() << "/**** ECALL prototypes. ****/";
        trusted_prototypes();
        out() << "/**** Untrusted function IDs. ****/";
        untrusted_function_ids();
        out() << "/**** OCALL marshalling structs. ****/";
        ocall_marshalling_structs();
        out() << "/**** OCALL prototypes. ****/";
        untrusted_prototypes();
        out() << "OE_EXTERNC_END"
              << "";
        footer(out(), guard);
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

    void trusted_prototypes()
    {
        for (Function* f : edl_->trusted_funcs_)
            out() << prototype(f, true, gen_t_h_) + ";"
                  << "";
        if (edl_->trusted_funcs_.empty())
            out() << "";
    }

    void untrusted_prototypes()
    {
        for (Function* f : edl_->untrusted_funcs_)
            out() << prototype(f, false, gen_t_h_) + ";"
                  << "";
        if (edl_->untrusted_funcs_.empty())
            out() << "";
    }
};

#endif // H_EMITTER_H
