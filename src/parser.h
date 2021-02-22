// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "warnings.h"

class Parser
{
    static std::vector<std::string> stack_;
    static std::map<std::string, Edl*> cache_;

    std::string filename_;
    std::string basename_;
    std::vector<std::string> searchpaths_;
    std::vector<std::string> defines_;
    std::unordered_map<Warning, WarningState, WarningHash> warnings_;

    Lexer* lex_;
    Token t_;
    Token t1_;
    int line_;
    int col_;
    bool in_struct_;
    bool in_function_;
    bool experimental_;

    std::vector<std::string> includes_;
    std::vector<UserType*> types_;
    std::vector<Function*> trusted_funcs_;
    std::vector<Function*> untrusted_funcs_;
    std::vector<Function*> imported_trusted_funcs_;
    std::vector<Function*> imported_untrusted_funcs_;

    Preprocessor pp_;
    enum AttrTok
    {
        TokIn,
        TokOut,
        TokCount,
        TokSize,
        TokIsAry,
        TokIsPtr,
        TokString,
        TokWstring,
        TokUserCheck
    };
    std::vector<std::pair<AttrTok, Token>> attr_toks_;

  private:
    Token get_preprocessed_token();
    Token next();
    Token peek();
    Token peek1();

    bool print_loc(
        const std::string& msg_kind,
        const char* filename,
        int line,
        int col);

    void parse_include();
    void parse_import();
    void parse_from_import();
    Edl* parse_import_file();
    void parse_enum();
    void parse_struct_or_union(bool is_struct);
    void parse_trusted();
    void parse_untrusted();
    Attrs* parse_attributes();
    Decl* parse_decl();
    void parse_allow_list(bool trusted, const std::string& fname);
    Function* parse_function_decl(bool trusted = true);
    Type* parse_atype();
    Type* parse_atype1(Token t);
    Type* parse_atype2(Token t);
    Dims* parse_dims();

  private:
    void append_include(const std::string& inc);
    void append_type(UserType* type);
    void append_function(std::vector<Function*>& funcs, Function* f);
    void warn_or_err(Warning option, Token* token, const char* fmt, ...);
    void warn_unsupported_allow(const std::string& fname);
    void check_non_portable_type(Function* function);
    void warn_non_portable_type(
        const std::string& fname,
        const std::string& type);
    void warn_function_return_ptr(const std::string& fname, Type* t);
    void warn_ptr_in_local_struct(const std::string& sname, Decl* d);
    void warn_signed_size_or_count(
        Token* token,
        const std::string& type,
        const std::string& name,
        const std::string& param);
    void check_function_param(const std::string& fname, Decl* d);
    void warn_foreign_ptr(
        const std::string& fname,
        const std::string& type,
        const std::string& param);
    void warn_ptr_in_function(
        const std::string& fname,
        const std::string& param);
    void error_size_count(Function* f);
    void check_size_count_decls(
        const std::string& parent_name,
        const std::vector<Decl*>& decls);

    AttrTok check_attribute(Token t);
    void validate_attributes(Decl* d);
    void check_deep_copy_struct_by_value(Function* f);

  private:
    void expect(const char* str);
    Edl* parse_body();

  public:
    Parser(
        const std::string& filename,
        const std::vector<std::string>& searchpaths,
        const std::vector<std::string>& defines,
        const std::unordered_map<Warning, WarningState, WarningHash>& warnings,
        bool experimental);
    ~Parser();

    Edl* parse();
};

#endif // PARSER_H
