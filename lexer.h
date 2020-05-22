// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef lexer_h
#define lexer_h

#include <cctype>
#include <cstring>
#include <string>

struct Token
{
    int line_;
    int col_;
    const char* start_;
    const char* end_;

    bool operator==(const char* str) const
    {
        size_t tok_len = static_cast<size_t>(end_ - start_);
        size_t str_len = static_cast<size_t>(strlen(str));
        return (tok_len == str_len) && (strncmp(start_, str, tok_len) == 0);
    }

    bool operator!=(const char* str) const
    {
        return !(*this == str);
    }

    bool operator==(const char ch) const
    {
        return *start_ == ch;
    }

    bool operator!=(const char ch) const
    {
        return *start_ != ch;
    }

    bool starts_with(const char* str) const
    {
        return strncmp(start_, str, strlen(str)) == 0;
    }

    bool is_eof() const
    {
        return *start_ == '\0';
    }

    bool is_name() const
    {
        return std::isalpha(*start_) || *start_ == '_';
    }

    bool is_int() const
    {
        return std::isdigit(*start_);
    }

    bool is_empty() const
    {
        return is_eof();
    }

    operator std::string() const
    {
        return is_empty() ? "" : std::string(start_, end_);
    }

    static Token empty()
    {
        const char* str = "\0";
        return Token{0, 0, str, str + 1};
    }
};

template <typename Os>
Os& operator<<(Os& os, const Token& t)
{
    return os << static_cast<std::string>(t);
}

class Lexer
{
    std::string filename_;
    const char* file_;
    const char* end_;
    const char* p_;
    int line_;
    int col_;

    void skip_ws();

  public:
    Lexer(const std::string& filename);
    ~Lexer();

    Token next();
};

#endif // lexer_h
