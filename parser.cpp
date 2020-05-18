// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <algorithm>

#include "parser.h"
#include "utils.h"

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

std::vector<std::string> Parser::stack_;

Parser::Parser(
    const std::string& filename,
    const std::vector<std::string>& searchpaths)
    : filename_(filename),
      basename_(),
      searchpaths_(searchpaths),
      lex_(),
      t_(),
      line_(1),
      col_(1),
      includes_(),
      types_(),
      trusted_funcs_(),
      untrusted_funcs_(),
      imported_trusted_funcs_(),
      imported_untrusted_funcs_()
{
    std::string f = filename_;
    if (!_is_file(f))
    {
        for (auto&& sp : searchpaths_)
        {
            f = sp + '/' + filename_;
            if (_is_file(f))
                break;
        }
    }
    if (!_is_file(f))
    {
        printf(
            "error: File not found within search paths: %s\n",
            filename_.c_str());
        exit(1);
    }

    size_t p = f.rfind('/');
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
    next();
}

Parser::~Parser()
{
}

Token Parser::peek()
{
    return t_;
}

Token Parser::next()
{
    Token t = t_;
    t_ = lex_->next();
    line_ = t.line_;
    col_ = t.col_;
    return t;
}

bool Parser::print_loc(const char* msg_kind)
{
    printf("%s: %s:%d:%d ", msg_kind, filename_.c_str(), line_, col_);
    return true;
}

#define ERROR(fmt, ...) \
do { \
    print_loc("error"); \
    printf(fmt, ##__VA_ARGS__); \
    printf("\n"); \
    exit(1);		      \
} while (0)

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
    if (in(filename_, stack_))
    {
        printf("recursive import detected\n");
        for (auto itr = stack_.rbegin(); itr != stack_.rend(); ++itr)
            printf("%s\n", itr->c_str());
        exit(1);
    }

    stack_.push_back(filename_);
    expect("enclave");
    expect("{");
    Edl* edl = parse_body();
    edl->name_ = basename_;
    expect("}");
    stack_.pop_back();
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

    includes_.push_back(t);
}

Edl* Parser::parse_import_file()
{
    Token t = next();
    if (!t.starts_with("\""))
	ERROR("expecting edl filename");

    Parser p (std::string(t.start_+1, t.end_-1), searchpaths_);
    Edl* edl = p.parse();
    return edl;
}

void Parser::parse_import()
{
    Edl* edl = parse_import_file();
    append(types_, edl->types_);
    append(includes_, edl->includes_);
    append(imported_trusted_funcs_, edl->trusted_funcs_);
    append(imported_untrusted_funcs_, edl->untrusted_funcs_);
    delete edl;
}

void Parser::parse_from_import()
{
    Edl* edl = parse_import_file();
    append(types_, edl->types_);
    append(includes_, edl->includes_);

    expect("import");
    if (peek() == '*')
    {
        next();
        append(imported_trusted_funcs_, edl->trusted_funcs_);
        append(imported_untrusted_funcs_, edl->untrusted_funcs_);
    }
    else
    {
        while (peek() != ';' && !peek().is_eof())
        {
            Token t = next();
            if (!t.is_name())
                ERROR("expecting function name");

            bool match = false;
            for (Function* f : edl->trusted_funcs_)
            {
                if (t == f->name_.c_str())
                {
                    imported_trusted_funcs_.push_back(f);
                    match = true;
                    break;
                }
            }

            for (Function* f : edl->untrusted_funcs_)
            {
                if (t == f->name_.c_str())
                {
                    imported_untrusted_funcs_.push_back(f);
                    match = true;
                    break;
                }
            }

            if (!match)
            {
                std::string n = t;
                ERROR("function %s not found in imported edl.", n.c_str());
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

    UserType* type = new UserType { enum_name, Enum, {}, {} };
    expect("{");
    while (peek() != '}')
    {
	Token name = next();
	if (!name.is_name())
	    ERROR("expecting identifier, got %s",
		  static_cast<std::string>(name).c_str());
	Token* value = nullptr;
	if (peek() == '=')
	{
	    next();
	    value = new Token(next());
	    if (!value->is_name() && !value->is_int())
		ERROR("expecting enum value, got %s",
		      static_cast<std::string>(*value).c_str());
	}
	if (peek() != '}')
	    expect(",");
	type->items_.push_back(EnumVal{name, value});
    }
    types_.push_back(type);
    expect("}");
    expect(";");
}

void Parser::parse_struct_or_union(bool is_struct)
{
    Token name = next();
    if (!name.is_name())
	ERROR("expecting struct/union name, got %s",
	      static_cast<std::string>(name).c_str());
    
    UserType* type = new UserType { name, is_struct ? Struct : Union, {}, {}};
    expect("{");
    while (peek() != "}")
    {
	Decl* decl = parse_decl(false);
	if (decl->attrs_ && !is_struct)
	    ERROR("attributes are not allowed for unions");
	type->fields_.push_back(decl);
	if (peek() != "}")
	    expect(";");
    }
    types_.push_back(type);
    expect("}");
    expect(";");
}

void Parser::parse_trusted()
{
    expect("{");
    while (peek() != '}')
    {
	expect("public"); // OE does not support private.
	trusted_funcs_.push_back(parse_function_decl(true));
    }
    expect("}");
    expect(";");
}

void Parser::parse_untrusted()
{
    expect("{");
    while (peek() != '}')
    {
        untrusted_funcs_.push_back(parse_function_decl(false));
    }
    expect("}");
    expect(";");
}

Function* Parser::parse_function_decl(bool trusted)
{
    Function* f = new Function { {}, {}, {}, false, false };
    f->rtype_ = parse_atype();
    Token name = next();
    if (!name.is_name())
	ERROR("expecting function name, got %s",
	      static_cast<std::string>(name).c_str());
    f->name_ = name;
    
    expect("(");
    while (peek() != ')')
    {
	f->params_.push_back(parse_decl(true));
	if (peek() != ')')
	    expect(",");
    }
    expect(")");
    for (int i=0; i < 2; ++i)
    {
	if (peek() == "transition_using_threads" && !f->switchless_)
	{
	    next();
	    f->switchless_ = true;	
	}
	else if(!trusted && peek() == "propagate_errno" && !f->errno_)
	{
	    next();
	    f->errno_= true;
	}
    }            
    expect(";");
    return f;
}


Decl* Parser::parse_decl(bool fcn)
{
    Decl* decl = new Decl { {}, {}, {}, nullptr };
    decl->attrs_ = parse_attributes(fcn);
    decl->type_ = parse_atype();
    Token name = next();
    if (!name.is_name())
	ERROR("expecting identifier got %s",
	      static_cast<std::string>(name).c_str());
    decl->name_ = name;
    decl->dims_ = parse_dims();
    return decl;
}

static bool _no_kind(const Attrs* a)
{
    return !(a->isptr_ || a->isary_ || a->string_ || a->wstring_);
}

Attrs* Parser::parse_attributes(bool fcn)
{
    if (peek() != '[')
        return nullptr;
    next();
    Attrs* attrs = new Attrs{false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             Token::empty(),
                             Token::empty()};
    do
    {
        Token t = next();
        if (fcn && t == "in" && !(attrs->in_ || attrs->inout_))
            *(attrs->out_ ? &attrs->inout_ : &attrs->in_) = true;
        else if (fcn && t == "out" && !(attrs->out_ || attrs->inout_))
            *(attrs->in_ ? &attrs->inout_ : &attrs->out_) = true;
        else if (fcn && t == "string" && _no_kind(attrs))
            attrs->string_ = true;
        else if (fcn && t == "wstring" && _no_kind(attrs))
            attrs->wstring_ = true;
        else if (fcn && t == "isptr" && _no_kind(attrs))
            attrs->isptr_ = true;
        else if (fcn && t == "isary" && _no_kind(attrs))
            attrs->isary_ = true;
        else if (t == "user_check" && !attrs->user_check_)
            attrs->user_check_ = true;
        else if (t == "count" || t == "size")
        {
            expect("=");
            Token v = next();
            if (!v.is_name() && !v.is_int())
                ERROR("expecting integer");
            if (t == "count")
            {
                if (!attrs->count_.is_empty())
                    ERROR("multiple count specifier");
                attrs->count_ = v;
            }
            else
            {
                if (!attrs->size_.is_empty())
                    ERROR("multiple size specifier");
                attrs->size_ = v;
            }
        }
        else
            ERROR(
                "expecting attribute got %s",
                static_cast<std::string>(t).c_str());

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
	ERROR("expecting typename, got %s",
	      static_cast<std::string>(t).c_str());

    if (const_)
	type = new Type { Const, type, {} };

    while (peek() == '*')
    {
	next();
	type = new Type { Ptr, type, {}};
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
	    return new Type { Unsigned, new Type { Int, nullptr, {}}, {}};
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
	type = new Type { (t== "short") ? Short : Char, nullptr, {}};
	if (peek() == "int")
	    next();
    }
    else if (t == "int")
	type = new Type { Int, nullptr, {} };

    if (unsigned_)
	type = new Type { Unsigned, type, {} };

    return type;
}

#define MATCH(str, tag) \
    if (t == str) \
	return new Type { tag, nullptr, {} }

Type* Parser::parse_atype2(Token t)
{
    if (t == "struct" || t == "enum" || t == "union")
    {
	Token name = next();
	if (!name.is_name())
	    ERROR("expecting struct/enum/union name, got %s",
		  static_cast<std::string>(name).c_str());

	AType at = Struct;
	if (t == "enum")
	    at = Enum;
	else if (t == "union")
	    at = Union;
	return new Type { at, nullptr, name }; 
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
