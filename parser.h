// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

class Parser
{
    static std::vector<std::string> stack_;

    std::string filename_;
    std::string basename_;
    std::vector<std::string> searchpaths_;

    Lexer* lex_;
    Token t_;
    Token t1_;
    int line_;
    int col_;

    std::vector<std::string> includes_;
    std::vector<UserType*> types_;
    std::vector<Function*> trusted_funcs_;
    std::vector<Function*> untrusted_funcs_;
    std::vector<Function*> imported_trusted_funcs_;
    std::vector<Function*> imported_untrusted_funcs_;

  private:
    Token next();
    Token peek();
    Token peek1();

    bool print_loc(const char* msg_kind);

    void parse_include();
    void parse_import();
    void parse_from_import();
    Edl* parse_import_file();
    void parse_enum();
    void parse_struct_or_union(bool is_struct);
    void parse_trusted();
    void parse_untrusted();
    Attrs* parse_attributes(bool fcn = true);
    Decl* parse_decl(bool fcn = true);
    void parse_allow_list(bool trusted, const std::string& fname);
    Function* parse_function_decl(bool trusted = true);
    Type* parse_atype();
    Type* parse_atype1(Token t);
    Type* parse_atype2(Token t);
    Dims* parse_dims();

  private:
    void warn_allow_list(const std::string& fname);
    void warn_non_portable(Function* f);
    void error_size_count(Function* f);
    void check_size_count_decls(
        const std::string& parent_name,
        bool is_function,
        const std::vector<Decl*>& decls);

  private:
    void expect(const char* str);
    Edl* parse_body();

  public:
    Parser(
        const std::string& filename,
        const std::vector<std::string>& searchpaths);
    ~Parser();

    Edl* parse();
};

#endif // PARSER_H
