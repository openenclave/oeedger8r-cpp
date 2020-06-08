// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include "lexer.h"
#include <ctype.h>
#include <stdio.h>

Lexer::Lexer(const std::string& file)
    : filename_(file), file_(0), p_(0), line_(0), col_(0)
{
    FILE* f = NULL;
#if _WIN32
    fopen_s(&f, file.c_str(), "rb");
#else
    f = fopen(file.c_str(), "rb");
#endif
    if (!f)
    {
        fprintf(stderr, "error: cannot open file %s\n", file.c_str());
        exit(1);
    };
    fseek(f, 0, SEEK_END);
    size_t len = static_cast<size_t>(ftell(f));
    char* contents = (char*)malloc(len + 1);
    fseek(f, 0, SEEK_SET);
    if (fread(contents, 1, len, f) != len)
    {
        fprintf(stderr, "error reading %s\n", file.c_str());
        exit(1);
    }
    fclose(f);
    contents[len] = '\0';

    file_ = contents;
    end_ = file_ + len;
    line_ = 1;
    col_ = 1;
    p_ = file_;
}

Lexer::~Lexer()
{
    // file_ is intended to out-live the lexer.
    // It is not freed.
}

void Lexer::skip_ws()
{
    while (p_ < end_)
    {
        switch (*p_)
        {
            case '\t':
                col_ += 4;
                ++p_;
                continue;
            case ' ':
                col_++;
                ++p_;
                continue;
            case '\n':
                line_++;
                col_ = 1;
                ++p_;
                continue;
            case '\r':
            case '\b':
            case '\v':
                ++p_;
                continue;
        }

        if (*p_ == '/' && p_ + 1 < end_)
        {
            if (p_[1] == '/')
            {
                // Single line comment.
                while (*p_ && *p_ != '\n')
                    ++p_;
                continue;
            }
            if (p_[1] == '*')
            {
                p_ += 2;
                while (p_ + 1 < end_ && !(p_[0] == '*' && p_[1] == '/'))
                {
                    ++col_;
                    if (*p_ == '\n')
                    {
                        line_++;
                        col_ = 1;
                    }
                    ++p_;
                }
                if (p_ + 1 >= end_)
                {
                    fprintf(
                        stderr,
                        "error: %s: EOF while looking for */\n",
                        filename_.c_str());
                    exit(1);
                }
                p_ += 2;
                continue;
            }
        }
        break;
    }
}

Token Lexer::next()
{
    skip_ws();

    switch (*p_)
    {
        case '{':
        case '}':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case ',':
        case ';':
        case '=':
        {
            Token t = {line_, col_, p_, p_ + 1};
            p_++;
            col_++;
            return t;
        }
        case '\0':
        {
            Token t = {line_, col_, p_, p_ + 1};
            return t;
        }
    }

    if (std::isalpha(*p_) || *p_ == '_')
    {
        Token t = {line_, col_, p_, p_ + 1};
        while (isalnum(*++p_) || *p_ == '_')
            ;
        t.end_ = p_;
        col_ += static_cast<int>(p_ - t.start_);
        return t;
    }

    if (std::isdigit(*p_))
    {
        Token t = {line_, col_, p_, p_ + 1};
        while (isdigit(*++p_))
            ;
        t.end_ = p_;
        col_ += static_cast<int>(p_ - t.start_);
        return t;
    }

    if (*p_ == '"')
    {
        Token t = {line_, col_, p_, p_ + 1};
        ++p_;
        while (*p_ && *p_ != '"' && *p_ != '\n')
            ++p_;
        if (*p_ != '"')
        {
            fprintf(
                stderr,
                "error: %s:%d:%d: expecting \"\n",
                filename_.c_str(),
                line_,
                col_);
            exit(1);
        }
        t.end_ = ++p_;
        col_ += static_cast<int>(p_ - t.start_);
        return t;
    }

    fprintf(
        stderr,
        "error: %s:%d:%d: Unexpected token at %.1s\n",
        filename_.c_str(),
        line_,
        col_,
        p_);
    exit(1);
}
