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

    void emit_t_h(const std::string& dir_with_sep = "")
    {
        gen_t_h_ = true;
        file_.open(dir_with_sep + edl_->name_ + "_t.h");
        indent_ = "";
        emit_h();
        file_.close();
    }

    void emit_u_h(
        const std::string& dir_with_sep = "",
        const std::string& prefix = "")
    {
        gen_t_h_ = false;
        file_.open(dir_with_sep + edl_->name_ + "_u.h");
        emit_h(prefix);
        file_.close();
    }

    void emit_h(const std::string& prefix = "")
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
        out() << "/**** ECALL prototypes. ****/";
        trusted_prototypes(prefix);
        out() << "/**** OCALL prototypes. ****/";
        untrusted_prototypes();
        out() << "OE_EXTERNC_END"
              << "";
        footer(out(), guard);
    }

    void trusted_prototypes(const std::string& prefix = "")
    {
        // Prefix is generated, if specified, only in _u.h.
        for (Function* f : edl_->trusted_funcs_)
            out() << prototype(f, true, gen_t_h_, prefix) + ";"
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
