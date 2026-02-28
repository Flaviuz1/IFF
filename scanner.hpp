#pragma once

#include <string>
#include <vector>

enum TokenType {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_CARET/* ^ */,
    TOKEN_PERCENT, TOKEN_QMARK, TOKEN_COLON/* : */,
    TOKEN_DOLSIGN,
    // One or two character tokens.
    TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_CARET_EQUAL, TOKEN_PERCENT_EQUAL,
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_SHIFT_LEFT, TOKEN_SHIFT_RIGHT,
    TOKEN_INTERP_START, TOKEN_INTERP_END,
    // Three character tokens.
    TOKEN_SHIFT_LEFT_EQUAL, TOKEN_SHIFT_RIGHT_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    TOKEN_BINARY, TOKEN_HEX, TOKEN_OCTAL,
    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUNC, TOKEN_IF, TOKEN_NULL, TOKEN_OR,
    TOKEN_RETURN, TOKEN_SUPER, TOKEN_SELF,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_IN, TOKEN_IS,
    TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_MATCH, TOKEN_CASE,
    TOKEN_IMPORT,
    TOKEN_ERROR,
    TOKEN_EOF
};

struct Token {
    TokenType type;
    const char* start;
    int length;
    int line;
};

struct Scanner {
    const char* start;
    const char* current;
    int line;
    // for string interpolation
    std::vector<Token> tokenQueue;
    int interpolationDepth;
};

void initScanner(const char* source);
Token scanToken();