// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <map>

#include "parser.h"
#include "preprocessor.h"
#include "utils.h"

// Stack of edl files being parsed.
std::vector<std::string> Parser::stack_;

// Cache for edl files that have already been parsed.
// Reimporting an edl would just return the preparsed edl.
std::map<std::string, Edl*> Parser::cache_;

static bool _is_file(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "r");
    if (f)
    {
        fclose(f);
        return true;
    }
    return false;
}

Parser::Parser(
    const std::string& filename,
    const std::vector<std::string>& searchpaths,
    const std::vector<std::string>& defines,
    const std::unordered_map<Warning, WarningState, WarningHash>& warnings,
    bool experimental)
    : filename_(filename),
      basename_(),
      searchpaths_(searchpaths),
      defines_(defines),
      warnings_(warnings),
      lex_(),
      t_(),
      line_(1),
      col_(1),
      in_struct_(false),
      in_function_(false),
      experimental_(experimental),
      includes_(),
      types_(),
      trusted_funcs_(),
      untrusted_funcs_(),
      imported_trusted_funcs_(),
      imported_untrusted_funcs_(),
      pp_(defines)
{
    std::string f = filename_;
    if (!_is_file(f))
    {
        for (auto&& sp : searchpaths_)
        {
            f = fix_path_separators(sp + path_sep() + filename_);
            if (_is_file(f))
                break;
        }
    }
    if (!_is_file(f))
    {
        fprintf(
            stderr,
            "error: file not found within search paths: %s\n",
            filename_.c_str());
        exit(1);
    }

    // Search for the right occurance of path sep.
    size_t p = f.rfind(path_sep());
    if (p != std::string::npos)
        basename_ =
            std::string(f.begin() + static_cast<ptrdiff_t>(p) + 1, f.end());
    else
        basename_ = filename_;

    p = basename_.rfind('.');
    if (p != std::string::npos)
        basename_ = std::string(
            basename_.begin(), basename_.begin() + static_cast<ptrdiff_t>(p));
    lex_ = new Lexer(f);

    t_ = get_preprocessed_token();
    t1_ = get_preprocessed_token();

    // Remember full path to file.
    filename_ = f;
}

Parser::~Parser()
{
}

Token Parser::peek()
{
    return t_;
}

Token Parser::peek1()
{
    return t1_;
}

Token Parser::next()
{
    Token t = t_;
    t_ = t1_;
    t1_ = get_preprocessed_token();
    line_ = t.line_;
    col_ = t.col_;
    return t;
}

bool Parser::print_loc(
    const std::string& msg_kind,
    const char* filename,
    int line,
    int col)
{
    if (msg_kind == "error" || msg_kind == "warning")
        fprintf(stderr, "%s: %s:%d:%d ", msg_kind.c_str(), filename, line, col);
    else
        printf("%s: %s:%d:%d ", msg_kind.c_str(), filename_.c_str(), line, col);
    return true;
}

#define ERROR_AT(t, format, ...)                                    \
    do                                                              \
    {                                                               \
        print_loc("error", filename_.c_str(), (t).line_, (t).col_); \
        fprintf(stderr, format, ##__VA_ARGS__);                     \
        fprintf(stderr, "\n");                                      \
        exit(1);                                                    \
    } while (0)

#define ERROR(format, ...)                                  \
    do                                                      \
    {                                                       \
        print_loc("error", filename_.c_str(), line_, col_); \
        fprintf(stderr, format, ##__VA_ARGS__);             \
        fprintf(stderr, "\n");                              \
        exit(1);                                            \
    } while (0)

#define WARNING_AT(t, format, ...)                                    \
    do                                                                \
    {                                                                 \
        print_loc("warning", filename_.c_str(), (t).line_, (t).col_); \
        fprintf(stderr, format, ##__VA_ARGS__);                       \
        fprintf(stderr, "\n");                                        \
    } while (0)

#define WARNING(format, ...)                                  \
    do                                                        \
    {                                                         \
        print_loc("warning", filename_.c_str(), line_, col_); \
        fprintf(stderr, format, ##__VA_ARGS__);               \
        fprintf(stderr, "\n");                                \
    } while (0)

Token Parser::get_preprocessed_token()
{
    Token t = lex_->next();

    while (t == "#")
    {
        t = lex_->next();
        if (t == "ifdef")
        {
            t = lex_->next();
            std::string name = static_cast<std::string>(t);
            if (!pp_.process(Ifdef, name))
                ERROR("unexpected error with #ifdef");
            t = lex_->next();
        }
        else if (t == "ifndef")
        {
            t = lex_->next();
            std::string name = static_cast<std::string>(t);
            if (!pp_.process(Ifndef, name))
                ERROR("unexpected error with #ifndef");
            t = lex_->next();
        }
        else if (t == "else")
        {
            if (!pp_.process(Else))
                ERROR("no previous #ifdef or #ifndef");
            t = lex_->next();
        }
        else if (t == "endif")
        {
            if (!pp_.process(Endif))
                ERROR("no previous #ifdef, #ifndef, or #else");
            t = lex_->next();
        }
        else
        {
            ERROR(
                "unsupported directive %s",
                static_cast<std::string>(t).c_str());
        }

        if (!pp_.is_included())
        {
            // Skip tokens till next preprocessor directive.
            while (t != "#" && !t.is_eof())
            {
                t = lex_->next();
            }
        }
    }
    return t;
}

void Parser::expect(const char* str)
{
    Token t = next();
    if (t != str)
        ERROR(
            " expecting %s got %*.*s\n",
            str,
            0,
            static_cast<int>(t.end_ - t.start_),
            t.start_);
}

Edl* Parser::parse()
{
    // Detect recursive imports.
    if (in(filename_, stack_))
    {
        fprintf(stderr, "error: recursive import detected\n");
        for (auto itr = stack_.rbegin(); itr != stack_.rend(); ++itr)
            fprintf(stderr, "%s\n", itr->c_str());
        exit(1);
    }

    // If the edl has already been parsed, return the cached result.
    if (cache_.count(filename_))
        return cache_[filename_];

    printf("Processing %s.\n", filename_.c_str());
    stack_.push_back(filename_);
    expect("enclave");
    expect("{");
    Edl* edl = parse_body();
    edl->name_ = basename_;
    expect("}");
    stack_.pop_back();

    // Update cache.
    cache_[filename_] = edl;
    return edl;
}

Edl* Parser::parse_body()
{
    while (peek() != '}' && peek() != '\0')
    {
        Token t = next();
        if (t == "trusted")
            parse_trusted();
        else if (t == "untrusted")
            parse_untrusted();
        else if (t == "include")
            parse_include();
        else if (t == "import")
            parse_import();
        else if (t == "enum")
            parse_enum();
        else if (t == "struct" || t == "union")
            parse_struct_or_union(t == "struct");
        else if (t == "from")
            parse_from_import();
        else
        {
            ERROR("unexpected token %s\n", static_cast<std::string>(t).c_str());
        }
    }

    if (!pp_.is_closed())
        ERROR("unterminated #ifdef or #ifndef");

    append(trusted_funcs_, imported_trusted_funcs_);
    append(untrusted_funcs_, imported_untrusted_funcs_);

    return new Edl{
        basename_, includes_, types_, trusted_funcs_, untrusted_funcs_};
}

void Parser::parse_include()
{
    Token t = next();
    if (!t.starts_with("\""))
        ERROR("expecting header filename");

    append_include(t);
}

Edl* Parser::parse_import_file()
{
    Token t = next();
    if (!t.starts_with("\""))
        ERROR("expecting edl filename");

    Edl* edl = nullptr;
    if (pp_.is_included())
    {
        Parser p(
            std::string(t.start_ + 1, t.end_ - 1),
            searchpaths_,
            defines_,
            warnings_,
            experimental_);
        edl = p.parse();
    }
    return edl;
}

void Parser::parse_import()
{
    Edl* edl = parse_import_file();
    /*
     * In the case that import is excluded by the directive,
     * parse_import_file will return nullptr. Fall through directly
     * as the following logic does not affect the progression of the
     * parser.
     */
    if (!edl)
        return;
    for (UserType* type : edl->types_)
        append_type(type);

    for (const std::string& inc : edl->includes_)
        append_include(inc);

    for (Function* f : edl->trusted_funcs_)
        append_function(imported_trusted_funcs_, f);
    for (Function* f : edl->untrusted_funcs_)
        append_function(imported_untrusted_funcs_, f);
}

template <typename T>
static T* lookup(const std::vector<T*>& vec, const std::string& name)
{
    for (T* item : vec)
        if (item->name_ == name)
            return item;
    return nullptr;
}

void Parser::append_include(const std::string& inc)
{
    if (std::find(includes_.begin(), includes_.end(), inc) == includes_.end())
        includes_.push_back(inc);
}

void Parser::append_type(UserType* type)
{
    std::string name = type->name_;
    UserType* t = lookup(types_, name);
    if (t && t != type)
        ERROR("Duplicate type definition detected for %s", name.c_str());
    if (!t)
        types_.push_back(type);
}

void Parser::append_function(std::vector<Function*>& funcs, Function* f)
{
    std::string name = f->name_;
    Function* trusted_f = lookup(trusted_funcs_, name);
    Function* untrusted_f = lookup(untrusted_funcs_, name);
    Function* imported_trusted_f = lookup(imported_trusted_funcs_, name);
    Function* imported_untrusted_f = lookup(imported_untrusted_funcs_, name);

    bool duplicate = (trusted_f && f != trusted_f) ||
                     (untrusted_f && f != untrusted_f) ||
                     (imported_trusted_f && imported_trusted_f != f) ||
                     (imported_untrusted_f && imported_untrusted_f != f);
    if (duplicate)
        ERROR("Duplicate function definition detected for %s", name.c_str());

    // If the function does not already exist, append.
    if (!trusted_f && !untrusted_f && !imported_trusted_f &&
        !imported_untrusted_f)
        funcs.push_back(f);
}

void Parser::parse_from_import()
{
    Edl* edl = parse_import_file();
    /*
     * In the case that import is excluded by the directive,
     * parse_import_file will return nullptr. We cannot fall through
     * directly as the following logic affects the progression of
     * the parser. Instead, checking if the edl is nullptr before
     * referencing it.
     */
    if (edl)
    {
        for (UserType* type : edl->types_)
            append_type(type);
        for (const std::string& inc : edl->includes_)
            append_include(inc);
    }

    expect("import");
    if (peek() == '*')
    {
        next();
        if (edl)
        {
            for (Function* f : edl->trusted_funcs_)
                append_function(imported_trusted_funcs_, f);
            for (Function* f : edl->untrusted_funcs_)
                append_function(imported_untrusted_funcs_, f);
        }
    }
    else
    {
        while (peek() != ';' && !peek().is_eof())
        {
            Token t = next();
            if (!t.is_name())
                ERROR("expecting function name");

            if (edl)
            {
                std::string function_name = t;
                Function* imported_f =
                    lookup(edl->trusted_funcs_, function_name);
                std::vector<Function*>* funcs = &imported_trusted_funcs_;

                if (!imported_f)
                {
                    imported_f = lookup(edl->untrusted_funcs_, function_name);
                    funcs = &imported_untrusted_funcs_;
                }

                if (imported_f)
                {
                    append_function(*funcs, imported_f);
                }
                else
                {
                    ERROR(
                        "function %s not found in imported edl.",
                        function_name.c_str());
                }
            }

            if (peek() != ';')
                expect(",");
        }
    }
    expect(";");
}

void Parser::parse_enum()
{
    std::string enum_name = "";
    if (peek().is_name())
        enum_name = next();

    UserType* type = new UserType{enum_name, Enum, {}, {}};
    expect("{");
    while (peek() != '}')
    {
        Token name = next();
        if (!name.is_name())
            ERROR(
                "expecting identifier, got %s",
                static_cast<std::string>(name).c_str());
        Token* value = nullptr;
        if (peek() == '=')
        {
            next();
            value = new Token(next());
            if (!value->is_name() && !value->is_int())
                ERROR(
                    "expecting enum value, got %s",
                    static_cast<std::string>(*value).c_str());
        }
        if (peek() != '}')
            expect(",");
        type->items_.push_back(EnumVal{name, value});
    }
    append_type(type);
    expect("}");
    expect(";");
}

void Parser::parse_struct_or_union(bool is_struct)
{
    in_struct_ = is_struct;
    Token name = next();
    if (!name.is_name())
        ERROR(
            "expecting struct/union name, got %s",
            static_cast<std::string>(name).c_str());

    UserType* type = new UserType{name, is_struct ? Struct : Union, {}, {}};
    expect("{");
    while (peek() != "}")
    {
        Decl* decl = parse_decl();
        if (decl->attrs_ && !is_struct)
            ERROR("attributes are not allowed for unions");
        if (is_struct && decl->type_->tag_ == Ptr &&
            !has_size_or_count_attr(decl))
            warn_ptr_in_local_struct(name, decl);
        type->fields_.push_back(decl);
        if (peek() != "}")
            expect(";");
    }
    append_type(type);
    check_size_count_decls(type->name_, type->fields_);
    expect("}");
    expect(";");
    in_struct_ = false;
}

void Parser::parse_trusted()
{
    expect("{");
    while (peek() != '}')
    {
        bool is_private = true;
        if (peek() == "public")
            is_private = (next(), false);

        append_function(trusted_funcs_, parse_function_decl(true));
        if (is_private)
        {
            // Report error consistent with current edger8r.
            // TODO: Report location.
            fprintf(
                stderr,
                "error: Function `%s': `private' specifier is not supported by "
                "oeedger8r\n",
                trusted_funcs_.back()->name_.c_str());
            exit(1);
        }
    }

    expect("}");
    expect(";");
}

void Parser::parse_untrusted()
{
    expect("{");
    while (peek() != '}')
    {
        append_function(untrusted_funcs_, parse_function_decl(false));
    }

    expect("}");
    expect(";");
}

void Parser::parse_allow_list(bool trusted, const std::string& fname)
{
    if (peek() == "allow")
    {
        if (trusted)
            ERROR("the `allow' syntax is invalid for a trusted function "
                  "(ECALL).");

        next();
        expect("(");
        while (peek() != ")")
        {
            Token t = next();
            if (!t.is_name())
                ERROR(
                    "expecting identifier, got %s",
                    static_cast<std::string>(t).c_str());
            if (peek() != ")")
                expect(",");
        }
        expect(")");

        if (!trusted)
            warn_unsupported_allow(fname);
    }
}

Function* Parser::parse_function_decl(bool trusted)
{
    in_function_ = true;
    Function* f = new Function{{}, {}, {}, false, false};
    f->rtype_ = parse_atype();
    Token name = next();
    if (!name.is_name())
        ERROR(
            "expecting function name, got %s",
            static_cast<std::string>(name).c_str());
    f->name_ = name;

    if (f->rtype_->tag_ == Ptr)
        warn_function_return_ptr(f->name_, f->rtype_);

    expect("(");

    // Handle (void)
    if (peek() == "void" && peek1() == ")")
        next();

    while (peek() != ')')
    {
        Decl* decl = parse_decl();
        check_function_param(f->name_, decl);
        f->params_.push_back(decl);
        if (peek() != ')')
            expect(",");
    }
    expect(")");
    parse_allow_list(trusted, f->name_);

    for (int i = 0; i < 2; ++i)
    {
        if (peek() == "transition_using_threads" && !f->switchless_)
        {
            next();
            f->switchless_ = true;
        }
        else if (!trusted && peek() == "propagate_errno" && !f->errno_)
        {
            next();
            f->errno_ = true;
        }
    }
    expect(";");

    check_non_portable_type(f);
    error_size_count(f);
    check_size_count_decls(f->name_, f->params_);
    check_deep_copy_struct_by_value(f);
    in_function_ = false;
    return f;
}

Decl* Parser::parse_decl()
{
    Decl* decl = new Decl{{}, {}, {}, nullptr};
    decl->attrs_ = parse_attributes();
    decl->type_ = parse_atype();
    Token name = next();
    if (!name.is_name())
        ERROR(
            "expecting identifier got %s",
            static_cast<std::string>(name).c_str());
    decl->name_ = name;
    decl->dims_ = parse_dims();
    validate_attributes(decl);
    return decl;
}

Parser::AttrTok Parser::check_attribute(Token t)
{
    if (t == "in")
        return TokIn;
    if (t == "out")
        return TokOut;
    if (t == "count")
        return TokCount;
    if (t == "size")
        return TokSize;
    if (t == "isptr")
        return TokIsPtr;
    if (t == "isary")
        return TokIsAry;
    if (t == "string")
        return TokString;
    if (t == "wstring")
        return TokWstring;
    if (t == "user_check")
        return TokUserCheck;
    if (t == "sizefunc")
        ERROR("The attribute 'sizefunc' is deprecated. Please use 'size' "
              "attribute instead.");
    ERROR("unknown attribute: `%s'", static_cast<std::string>(t).c_str());
    // Unreachable
    return TokIn;
}

Attrs* Parser::parse_attributes()
{
    if (peek() != '[')
        return nullptr;

    next();
    Attrs* attrs = new Attrs{
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        Token::empty(),
        Token::empty()};
    attr_toks_.clear();
    do
    {
        Token t = next();
        AttrTok atok = check_attribute(t);

        // Only count and size attributes are valid for struct properties.
        if (in_struct_ && (atok != TokCount && atok != TokSize))
            ERROR("only `count' and `size' attributes can be specified for "
                  "struct properties");

        // Check for duplicate specification.
        for (auto&& itr : attr_toks_)
        {
            if (itr.first == atok)
                ERROR(
                    "duplicated attribute: `%s'",
                    static_cast<std::string>(t).c_str());
        }
        attr_toks_.push_back(std::make_pair(atok, t));

        // Process the attribute.
        if (atok == TokCount || atok == TokSize)
        {
            expect("=");
            Token v = next();
            if (!v.is_name() && !v.is_int())
                ERROR("expecting integer");
            if (atok == TokCount)
                attrs->count_ = v;
            else
                attrs->size_ = v;
        }
        else if (atok == TokUserCheck)
            attrs->user_check_ = true;
        else if (atok == TokIn)
            *(attrs->out_ ? &attrs->inout_ : &attrs->in_) = true;
        else if (atok == TokOut)
            *(attrs->in_ ? &attrs->inout_ : &attrs->out_) = true;
        else if (atok == TokString)
            attrs->string_ = true;
        else if (atok == TokWstring)
            attrs->wstring_ = true;
        else if (atok == TokIsPtr)
            attrs->isptr_ = true;
        else if (atok == TokIsAry)
            attrs->isary_ = true;
        else
            ERROR(
                "unexpected token `%s''", static_cast<std::string>(t).c_str());

        // Check for sentinel.
        if (peek() != ']')
            expect(",");

    } while (peek() != ']');

    expect("]");

    return attrs;
}

Type* Parser::parse_atype()
{
    Token t = next();
    bool const_ = false;
    if (t == "const")
    {
        const_ = true;
        t = next();
    }

    Type* type = parse_atype1(t);
    if (!type)
        type = parse_atype2(t);
    if (!type)
        ERROR(
            "expecting typename, got %s", static_cast<std::string>(t).c_str());

    if (const_)
        type = new Type{Const, type, {}};

    while (peek() == '*')
    {
        next();
        type = new Type{Ptr, type, {}};
    }

    return type;
}

Type* Parser::parse_atype1(Token t)
{
    Type* type = nullptr;
    bool unsigned_ = false;
    if (t == "unsigned")
    {
        unsigned_ = true;
        Token p = peek();
        if (p == "char" || p == "short" || p == "int" || p == "long")
            t = next();
        else
            return new Type{Unsigned, new Type{Int, nullptr, {}}, {}};
    }

    if (t == "long")
    {
        Token p = peek();
        if (p == "int")
            (type = new Type{Long, nullptr, {}}, next());
        else if (p == "long")
            (type = new Type{LLong, nullptr, {}}, next());
        else if (p == "double")
        {
            if (unsigned_)
                ERROR("invalid double following unsigned");
            (type = new Type{LDouble, nullptr, {}}, next());
        }
        else
            type = new Type{Long, nullptr, {}};
    }
    else if (t == "short" || t == "char")
    {
        type = new Type{(t == "short") ? Short : Char, nullptr, {}};
        if (peek() == "int")
            next();
    }
    else if (t == "int")
        type = new Type{Int, nullptr, {}};

    if (unsigned_)
        type = new Type{Unsigned, type, {}};

    return type;
}

#define MATCH(str, tag)   \
    if (t == str)         \
        return new Type   \
        {                 \
            tag, nullptr, \
            {             \
            }             \
        }

Type* Parser::parse_atype2(Token t)
{
    if (t == "struct" || t == "enum" || t == "union")
    {
        Token name = next();
        if (!name.is_name())
            ERROR(
                "expecting struct/enum/union name, got %s",
                static_cast<std::string>(name).c_str());

        AType at = Struct;
        if (t == "enum")
            at = Enum;
        else if (t == "union")
            at = Union;
        return new Type{at, nullptr, name};
    }

    MATCH("bool", Bool);
    MATCH("void", Void);
    MATCH("wchar_t", WChar);
    MATCH("size_t", SizeT);
    MATCH("int8_t", Int8);
    MATCH("int16_t", Int16);
    MATCH("int32_t", Int32);
    MATCH("int64_t", Int64);
    MATCH("uint8_t", UInt8);
    MATCH("uint16_t", UInt16);
    MATCH("uint32_t", UInt32);
    MATCH("uint64_t", UInt64);
    MATCH("float", Float);
    MATCH("double", Double);

    if (t.is_name())
        return new Type{Foreign, {}, t};

    return nullptr;
}

Dims* Parser::parse_dims()
{
    if (peek() != '[')
        return nullptr;
    Dims* dims = new Dims();
    while (peek() == '[')
    {
        next();
        Token t = next();
        if (!t.is_int() && !t.is_name())
            ERROR(
                "expecting array dimension, got %s",
                static_cast<std::string>(t).c_str());
        dims->push_back(t);
        expect("]");
    }

    return dims;
}

void Parser::warn_or_err(Warning option, Token* token, const char* format, ...)
{
    WarningState state = WarningState::Unknown;
    if (warnings_.find(option) != warnings_.end())
        state = warnings_[option];

    /* Bypass the warning if the -W<option> is specified. */
    if (state == WarningState::Ignore)
        return;

    /* warnings_[Warning::All] == WarningState::Warning represents the -Wall
     * option. */
    if (state != WarningState::Unknown ||
        warnings_[Warning::All] == WarningState::Warning)
    {
        /* Arguments forwarding. */
        const size_t MESSAGE_SIZE = 256;
        char message[MESSAGE_SIZE] = {'\0'};
        va_list arguments;
        va_start(arguments, format);
        vsnprintf(message, MESSAGE_SIZE, format, arguments);
        va_end(arguments);

        /* warnings_[Warning::Error] == Warning represents the -Werror option.
         */
        if (state == WarningState::Error ||
            warnings_[Warning::Error] == WarningState::Warning)
        {
            if (token)
                ERROR_AT(*token, "%s", message);
            else
                ERROR("%s", message);
        }
        else
        {
            if (token)
                WARNING_AT(*token, "%s", message);
            else
                WARNING("%s", message);
        }
    }
}

void Parser::warn_unsupported_allow(const std::string& fname)
{
    const char* format =
        "Function `%s': the `allow' syntax is currently unsupported. "
        "Ignored [-Wunsupported-allow].";

    warn_or_err(Warning::UnsupportedAllow, nullptr, format, fname.c_str());
}

void Parser::check_non_portable_type(Function* f)
{
    std::string type;
    for (Decl* p : f->params_)
    {
        Type* t = p->type_;
        while (t->tag_ == Const || t->tag_ == Ptr)
            t = t->t_;

        if (t->tag_ == WChar)
            type = "wchar_t";
        else if (t->tag_ == LDouble)
            type = "long double";
        else if (t->tag_ == Long)
            type = "long";
        else if (t->tag_ == Unsigned && t->t_->tag_ == Long)
            type = "unsigned long";

        if (!type.empty())
        {
            warn_non_portable_type(f->name_, type);
            type.clear();
        }
    }
}

void Parser::warn_non_portable_type(
    const std::string& fname,
    const std::string& type)
{
    const char* format =
        "warning: Function `%s': `%s' has different sizes on Windows and "
        "Linux. This enclave cannot be built in Linux and then safely "
        "loaded in Windows [-Wnon-portable-type].";

    warn_or_err(
        Warning::NonPortableType, nullptr, format, fname.c_str(), type.c_str());
}

void Parser::warn_function_return_ptr(const std::string& fname, Type* t)
{
    const char* format = "Function `%s': The function returns a pointer, which "
                         "could expose memory "
                         "addresses across the host-enclave boundary. Consider "
                         "passing the pointer as "
                         "an out parameter instead [-Wreturn-ptr].";

    warn_or_err(Warning::ReturnPtr, nullptr, format, fname.c_str());
}

void Parser::warn_ptr_in_local_struct(const std::string& sname, Decl* d)
{
    const char* format =
        "struct `%s': The member `%s' is a pointer that is not serializable. "
        "Consider annotating the member with the `count' or `size' attribute "
        "[-Wptr-in-struct].";

    warn_or_err(
        Warning::PtrInStruct, nullptr, format, sname.c_str(), d->name_.c_str());
}

void Parser::check_function_param(const std::string& fname, Decl* d)
{
    if (d->attrs_ && d->attrs_->user_check_)
        return;

    Type* type = d->type_;
    /* Only check if the parameter is a pointer. */
    if (type->tag_ != Ptr)
        return;

    if (!d->attrs_)
        warn_ptr_in_function(fname, d->name_);

    /* Unwrap the pointer and const types. */
    type = type->t_;
    if (type->tag_ == Const)
        type = type->t_;

    /*
     * Warn if we do not have the local definition of the type.
     * Note that a foreign type can also be a struct.
     */
    UserType* ut = get_user_type(types_, type->name_);
    if ((type->tag_ == Foreign || type->tag_ == Struct) && !ut)
        warn_foreign_ptr(fname, type->name_, d->name_);
}

void Parser::warn_ptr_in_function(
    const std::string& fname,
    const std::string& param)
{
    const char* format =
        "Function `%s': `%s' is a pointer that is not serializable. "
        "Consider annotating the parameter with the direction annotation "
        "[-Wptr-in-function].";

    warn_or_err(
        Warning::PtrInFunction, nullptr, format, fname.c_str(), param.c_str());
}

void Parser::warn_foreign_ptr(
    const std::string& fname,
    const std::string& type,
    const std::string& param)
{
    const char* format =
        "Function `%s': `%s' is a pointer of a foreign type `%s' that may not "
        "be serializable. "
        "Consider defining the type in the EDL file with proper annotations "
        "[-Wforeign-type-ptr].";

    warn_or_err(
        Warning::ForeignTypePtr,
        nullptr,
        format,
        fname.c_str(),
        param.c_str(),
        type.c_str());
}

void Parser::warn_signed_size_or_count(
    Token* token,
    const std::string& type,
    const std::string& name,
    const std::string& param)
{
    const char* format =
        "%s `%s': The size or count parameter `%s' should not be signed "
        "[-Wsigned-size-or-count].";

    warn_or_err(
        Warning::SignedSizeOrCount,
        token,
        format,
        type.c_str(),
        name.c_str(),
        param.c_str());
}

void Parser::error_size_count(Function* f)
{
    for (Decl* p : f->params_)
    {
        if (p->attrs_ && !p->attrs_->size_.is_empty() &&
            !p->attrs_->count_.is_empty())
        {
            fprintf(
                stderr,
                "error: Function `%s': simultaneous `size' and `count' "
                "parameters `size' and `count' are not supported by "
                "oeedger8r.",
                f->name_.c_str());
            exit(1);
        }
    }
}

static std::vector<Token> _get_size_or_count_attr_tokens(Decl* d)
{
    std::vector<Token> tokens;

    if (d->attrs_)
    {
        if (!d->attrs_->size_.is_empty())
            tokens.push_back(d->attrs_->size_);

        if (!d->attrs_->count_.is_empty())
            tokens.push_back(d->attrs_->count_);
    }

    return tokens;
}

static Decl* _get_decl(const std::vector<Decl*>& decls, const std::string& name)
{
    for (Decl* d : decls)
    {
        if (d->name_ == name)
            return d;
    }
    return nullptr;
}

void Parser::check_size_count_decls(
    const std::string& parent_name,
    const std::vector<Decl*>& decls)
{
    for (Decl* d : decls)
    {
        std::vector<Token> tokens = _get_size_or_count_attr_tokens(d);
        for (Token& t : tokens)
        {
            if (t.is_empty() || !t.is_name())
                continue;

            Decl* sc_decl = _get_decl(decls, t);
            if (sc_decl == nullptr)
                ERROR_AT(
                    t,
                    "could not find declaration for `%s'.",
                    static_cast<std::string>(t).c_str());

            /*
             * Setting the is_size_or_count_ attribute for the user-defined
             * struct only, which is relevant to the deep-copy support.
             */
            if (in_struct_)
            {
                /*
                 * The struct member used by the size or count attribute will
                 * not have attrs_ during parse_attributes(). Allocate a new
                 * one.
                 */
                if (sc_decl->attrs_ == nullptr)
                    sc_decl->attrs_ = new Attrs{
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        false,
                        Token::empty(),
                        Token::empty()};
                /*
                 * We can only be sure if a struct member is used by the size or
                 * count attribute after parsing; i.e., we cannot decide it
                 * during parsing if the member is defined after the size/count
                 * annotation. Therefore, we set the is_size_or_count_ attribute
                 * here, which prevents the member being modified by the
                 * callee during the deep copy.
                 */
                sc_decl->attrs_->is_size_or_count_ = true;
            }

            /* Validate the _is_size_or_count is correctly set for the struct.
             */
            if (in_struct_ && !sc_decl->attrs_ &&
                !sc_decl->attrs_->is_size_or_count_)
                ERROR_AT(
                    t,
                    "unexpected error with `%s'.",
                    static_cast<std::string>(t).c_str());

            Type* ty = sc_decl->type_;
            if (ty->tag_ == Const)
                ty = ty->t_;

            if (sc_decl->dims_ && !sc_decl->dims_->empty())
                ERROR_AT(t, "size/count has invalid type.");

            switch (ty->tag_)
            {
                case Unsigned:
                    continue;

                case Char:
                case Short:
                case Int:
                case Long:
                case LLong:
                case Int8:
                case Int16:
                case Int32:
                case Int64:
                {
                    warn_signed_size_or_count(
                        &t,
                        in_function_ ? "Function" : "struct",
                        parent_name.c_str(),
                        static_cast<std::string>(t));
                    continue;
                }

                case Ptr:
                case Struct:
                case Union:
                    ERROR_AT(t, "size/count has invalid type.");
                default:
                    break;
            }
        }
    }
}

void Parser::check_deep_copy_struct_by_value(Function* f)
{
    for (Decl* p : f->params_)
    {
        Type* type = p->type_;
        if (type->tag_ == Const)
            type = type->t_;
        if (type->tag_ != Struct && type->tag_ != Foreign)
            continue;

        UserType* ut = get_user_type(types_, type->name_);
        if (!ut)
            continue;

        for (Decl* field : ut->fields_)
        {
            if (field->attrs_)
            {
                fprintf(
                    stderr,
                    "error: the structure declaration `%s' specifies a "
                    "deep copy is expected. Referenced by value in function "
                    "`%s' detected.",
                    type->name_.c_str(),
                    f->name_.c_str());
                exit(1);
            }
        }
    }
}

void Parser::validate_attributes(Decl* d)
{
    Attrs* attrs = d->attrs_;
    Type* type = d->type_;
    if (!attrs)
        return;

    bool isptr = type->tag_ == Ptr;
    bool isary = d->dims_ && !d->dims_->empty();
    Type* base = nullptr;
    if (isptr)
    {
        base = type->t_;
        if (base->tag_ == Const)
            base = base->t_;
    }

    for (auto&& itr : attr_toks_)
    {
        if (itr.first == TokString || itr.first == TokWstring)
        {
            if (attrs->out_)
                ERROR_AT(
                    itr.second,
                    "string/wstring attribute should be used with an `in' "
                    "attribute");

            if (!attrs->in_ && !attrs->inout_)
                ERROR_AT(
                    itr.second,
                    "string/wstring attributes must be used with pointer "
                    "direction");

            if (!attrs->count_.is_empty() || !attrs->size_.is_empty())
                ERROR_AT(
                    itr.second,
                    "size attributes are mutually exclusive with (w)string "
                    "attribute");

            if (attrs->string_ && attrs->wstring_)
                ERROR_AT(
                    itr.second,
                    "`string' and `wstring' are mutually exclusive");

            if (itr.first == TokString && !(isptr && base->tag_ == Char))
                ERROR_AT(
                    itr.second,
                    "invalid `string' attribute - `%s' is not char pointer",
                    d->name_.c_str());

            if (itr.first == TokWstring && !(isptr && base->tag_ == WChar))
                ERROR_AT(
                    itr.second,
                    "invalid `wstring' attribute - `%s' is not wchar_t pointer",
                    static_cast<std::string>(d->name_).c_str());
        }

        else if (itr.first == TokIsAry || itr.first == TokIsPtr)
        {
            const char* tokstr =
                (itr.first == TokIsAry) ? "`isary'" : "`isptr'";

            if (attrs->isary_ && attrs->isptr_)
                ERROR_AT(
                    itr.second, "`isary' cannot be used with `isptr' together");

            if (!attrs->in_ && !attrs->inout_ && !attrs->out_ &&
                !attrs->user_check_)
                ERROR_AT(
                    itr.second,
                    "%s should have direction attribute or `user_check'",
                    tokstr);

            if (type->tag_ != Foreign)
            {
                std::string typestr = atype_str(d->type_);
                ERROR_AT(
                    itr.second,
                    "%s attribute is only valid for user defined type, not for "
                    "`%s'",
                    tokstr,
                    typestr.c_str());
            }

            if (itr.first == TokIsPtr && d->dims_)
            {
                ERROR_AT(
                    itr.second,
                    "invalid parameter - `%s' is a pointer array",
                    static_cast<std::string>(d->name_).c_str());
            }
        }

        else if (itr.first == TokCount || itr.first == TokSize)
        {
            if (in_function_)
            {
                if (!attrs->in_ && !attrs->inout_ && !attrs->out_)
                    ERROR_AT(
                        itr.second,
                        "size/count attributes must be used with pointer "
                        "direction");
            }

            if (!isptr && !attrs->isptr_ && !isary && !attrs->isary_)
            {
                std::string t = atype_str(d->type_);
                if (d->type_->tag_ == Foreign)
                {
                    const char* tokstr =
                        (itr.first == TokCount) ? "count" : "size";
                    std::string typestr = atype_str(d->type_);
                    ERROR_AT(
                        itr.second,
                        "`%s' is invalid for plain type `%s'",
                        tokstr,
                        typestr.c_str());
                }
                else
                {
                    ERROR_AT(
                        itr.second,
                        "unexpected pointer attributes for `%s'",
                        atype_str(type).c_str());
                }
            }
        }

        else if (itr.first == TokIn || itr.first == TokOut)
        {
            if (!isptr && !attrs->isptr_ && !isary && !attrs->isary_)
            {
                std::string t = atype_str(d->type_);
                if (d->type_->tag_ == Foreign)
                {
                    const char* tokstr = (itr.first == TokIn) ? "in" : "out";
                    std::string typestr = atype_str(d->type_);
                    ERROR_AT(
                        itr.second,
                        "`%s' is invalid for plain type `%s'",
                        tokstr,
                        typestr.c_str());
                }
                else
                {
                    ERROR_AT(
                        itr.second,
                        "unexpected pointer attributes for `%s'",
                        atype_str(type).c_str());
                }
            }

            if (d->type_->tag_ == Ptr)
            {
                UserType* ut = get_user_type_for_deep_copy(types_, d);
                if (ut)
                {
                    if (!attrs->size_.is_empty())
                        ERROR_AT(
                            itr.second,
                            "size attributes are invalid for the pointer of an "
                            "user-defined type `%s'",
                            ut->name_.c_str());
                }

                if (d->dims_)
                {
                    ERROR_AT(
                        itr.second,
                        "invalid parameter - `%s' is a pointer array",
                        static_cast<std::string>(d->name_).c_str());
                }
            }
        }

        else if (itr.first == TokUserCheck)
        {
            std::string typestr = atype_str(d->type_);
            if (attrs->in_ || attrs->out_ || attrs->inout_)
                ERROR_AT(
                    itr.second,
                    "pointer direction and `user_check' are mutually "
                    "exclusive");

            if (!attrs->isptr_ && !attrs->isary_ && !isary && !isptr)
                ERROR_AT(
                    itr.second,
                    "`user_check' attribute is invalid for plain type `%s'",
                    typestr.c_str());
        }
    }
}
