// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef ARGS_H_EMITTER_H
#define ARGS_H_EMITTER_H

#include <fstream>

#include "ast.h"
#include "utils.h"

class ArgsHEmitter
{
    Edl* edl_;
    std::ofstream file_;

  public:
    typedef ArgsHEmitter& R;

    template <typename T>
    R operator<<(const T& t)
    {
        file_ << t << "\n";
        return out();
    }

    R out()
    {
        return *this;
    }

  public:
    ArgsHEmitter(Edl* edl) : edl_(edl), file_()
    {
    }

    void emit(const std::string& dir_with_sep = "")
    {
        std::string path = dir_with_sep + edl_->name_ + "_args.h";
        file_.open(path.c_str(), std::ios::out | std::ios::binary);
        std::string guard = "EDGER8R_" + upper(edl_->name_) + "_ARGS_H";
        header(out(), guard);
        out() << ""
              << "#include <openenclave/bits/result.h>"
              << ""
              << "/**** User includes. ****/";
        user_includes();
        out() << "/**** User defined types in EDL. ****/";
        user_types();
        footer(out(), guard);
        file_.close();
    }

    void user_includes()
    {
        if (edl_->includes_.empty())
            out() << "/* There were no user includes. */";
        else
            for (std::string& inc : edl_->includes_)
                out() << "#include " + inc;
        out() << "";
    }

    void user_types()
    {
        if (edl_->types_.empty())
            out() << "/* There were no user defined types. */"
                  << "";
        else
            for (UserType* t : edl_->types_)
            {
                if (t->tag_ == Enum)
                    enum_type(t);
                else
                    struct_or_union_type(t);
            }
    }

    void enum_type(UserType* t)
    {
        std::string uname = upper(t->name_);
        // clang-format off
        out() << "#ifndef EDGER8R_ENUM_" + uname
              << "#define EDGER8R_ENUM_" + uname
	      << "typedef enum " + t->name_
              << "{";
        // clang-format on
        size_t i = 0;
        for (EnumVal& v : t->items_)
        {
            std::string value =
                v.value_ ? (" = " + static_cast<std::string>(*v.value_)) : "";
            out() << "    " + v.name_ + value +
                         (i < t->items_.size() - 1 ? "," : "");
            ++i;
        }
        out() << "} " + t->name_ + ";"
              << "#endif"
              << "";
    }
    void struct_or_union_type(UserType* t)
    {
        std::string tag = t->tag_ == Struct ? "struct" : "union";
        std::string utag = upper(tag);
        std::string uname = upper(t->name_);
        // clang-format off
        out() << "#ifndef EDGER8R_" + utag + "_" + uname
              << "#define EDGER8R_" + utag + "_" + uname
	      << "typedef " + tag + " " + t->name_
              << "{";
        // clang-format on
        for (Decl* f : t->fields_)
            out() << "    " + decl_str(f->name_, f->type_, f->dims_) + ";";
        out() << "} " + t->name_ + ";"
              << "#endif"
              << "";
    }
};

#endif // ARGS_H_EMITTER
