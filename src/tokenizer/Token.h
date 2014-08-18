#ifndef TOKEN_H
#define TOKEN_H
#include "swift_types.h"
#include <vector>
#include <cstring>
#include <string>
#include <wchar.h>

SWIFT_NS_BEGIN

struct TokenizerState : SourceInfo
{

    int cursor;
    bool hasSpace;
    bool inStringExpression;
};

struct Token
{
    //“Operators are made up of one or more of the following characters: /, =, -, +, !, *, %, <, >, &, |, ^, ~, and .. That said, the tokens =, ->, //, /*, */, ., and the unary prefix operator & are reserved. ”
    union
    {
        struct
        {
            bool multiline;
            int nestedLevels;
        } comment;
        struct
        {
            KeywordType::T type;
            Keyword::T keyword;
            bool backtick;
            bool implicitParameterName;
        } identifier;
        struct
        {
            bool expressionFollowed;
        } string;
        struct
        {
            int base;
            bool sign;
            int fraction;
            int exponent;

            int64_t value;
            double dvalue;
        } number;
        struct
        {
            OperatorType::T type;
        } operators;
    };
    TokenType::T type;
    std::wstring token;
    size_t size;
    TokenizerState state;
    void append(wchar_t ch)
    {
        token.push_back(ch);
        size++;
    }
    Keyword::T getKeyword() const
    {
        if(type != TokenType::Identifier)
            return Keyword::_;
        return identifier.keyword;
    }
    
    bool operator ==(const wchar_t*str) const
    {
        return wcscmp(token.c_str(), str) == 0;
    }
    bool operator ==(TokenType::T type) const
    {
        return this->type == type;
    }
};

SWIFT_NS_END
#endif//TOKEN_H