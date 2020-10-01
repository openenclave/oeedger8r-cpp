// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

#include "lexer.h"

enum AType
{
    Bool,
    Char,
    Short,
    Int,
    Long,
    LLong,
    Float,
    Double,
    LDouble,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Void,
    WChar,
    SizeT,
    Struct,
    Union,
    Enum,
    Foreign,
    Ptr,
    Const,
    Unsigned
};

struct Type
{
    AType tag_;
    Type* t_;
    std::string name_;
};

struct Attrs
{
    bool in_;
    bool out_;
    bool inout_;
    bool isptr_;
    bool isary_;
    bool string_;
    bool wstring_;
    bool user_check_;
    bool is_size_or_count_;
    Token size_;
    Token count_;
};

typedef std::vector<std::string> Dims;

struct Decl
{
    std::string name_;
    Type* type_;
    Dims* dims_;
    Attrs* attrs_;
};

struct EnumVal
{
    std::string name_;
    Token* value_;
};

struct UserType
{
    std::string name_;
    AType tag_;
    std::vector<Decl*> fields_;
    std::vector<EnumVal> items_;
};

struct Function
{
    std::string name_;
    Type* rtype_;
    std::vector<Decl*> params_;
    bool switchless_;
    bool errno_;
};

struct Edl
{
    std::string name_;
    std::vector<std::string> includes_;
    std::vector<UserType*> types_;
    std::vector<Function*> trusted_funcs_;
    std::vector<Function*> untrusted_funcs_;
};

enum Directive
{
    Ifdef,
    Ifndef,
    Else,
    Endif,
};

#endif // AST_H
