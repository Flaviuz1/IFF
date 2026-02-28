#include <cstdio>
#include <string>
#include <cstring>
#include <cctype>
#include "scanner.hpp"
#include "common.hpp"

Scanner scanner;

void initScanner(const char* source){
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.tokenQueue.clear();
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static bool isAlpha(char c) {
    return (isalpha(c) || c=='_');
}

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static void skipWhitespace(){
    for(;;){
        char c = peek();
        switch (c) {
            case ' ' :
            case '\r' :
            case '\t' : 
                advance(); 
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if(peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else if (peekNext() == '*') {
                    advance(); advance();
                    while (!isAtEnd()) {
                        if (peek() == '*' && peekNext() == '/') {
                            advance(); advance();
                            break;
                        }
                        if (peek() == '\n') scanner.line++;
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default : return;
        }
    }
}

static bool match(char expected) {
    if(isAtEnd()) return false;
    if(*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type){
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* msg){
    Token token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.length = strlen(msg);
    token.line = scanner.line;
    return token;
}

static Token dequeueToken() {
    Token token = scanner.tokenQueue.front();
    scanner.tokenQueue.erase(scanner.tokenQueue.begin());
    return token;
}

static Token string(char starting_type) {
    scanner.start = scanner.current;
    
    while (!isAtEnd()) {
        if (peek() == starting_type) break;
        if (peek() == '\n') scanner.line++;
        
        if (peek() == '$' && peekNext() == '{') {
            scanner.tokenQueue.push_back(makeToken(TOKEN_STRING));
            advance(); advance();
            scanner.interpolationDepth++;
            scanner.tokenQueue.push_back(makeToken(TOKEN_INTERP_START));
            return dequeueToken();
        }
        
        advance();
    }

    if(isAtEnd()) return errorToken("Unfinished string.");

    advance();
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while(isdigit(peek())) advance();
    
    if(peek() == '.' && isdigit(peekNext())){
        advance();
        while(isdigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if(scanner.current - scanner.start == start + length && 
       memcmp(scanner.start + start, rest, length) == 0) return type;

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': 
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
                    case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
            break;
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'i':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'f': return checkKeyword(2, 0, "", TOKEN_IF);
                    case 'n': return checkKeyword(2, 0, "", TOKEN_IN);
                    case 's': return checkKeyword(2, 0, "", TOKEN_IS);
                    case 'm': return checkKeyword(2, 4, "port", TOKEN_IMPORT);
                }
            }
            break;
        case 'm': return checkKeyword(1, 4, "atch", TOKEN_MATCH);
        case 'n': return checkKeyword(1, 3, "ull", TOKEN_NULL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': 
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e': return checkKeyword(2, 2, "lf", TOKEN_SELF);
                    case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
                }
            }
            break;
        case 't': return checkKeyword(1, 3, "rue", TOKEN_TRUE);
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while(isAlpha(peek()) || isdigit(peek())) advance();
    return makeToken(identifierType());
}

Token scanToken(){
    if (!scanner.tokenQueue.empty()) return dequeueToken();

    skipWhitespace();
    scanner.start = scanner.current;

    if(isAtEnd()) {
        if(scanner.interpolationDepth > 0) return errorToken("Unterminated string interpolation.");
        return makeToken(TOKEN_EOF);
    }

    // True Chad Patters Recognizer
    char c = advance();
    if (isdigit(c)) return number();
    if (isAlpha(c)) return identifier();

    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': if (scanner.interpolationDepth > 0) {scanner.interpolationDepth--; return makeToken(TOKEN_INTERP_END);} else return makeToken(TOKEN_RIGHT_BRACE);
        case '[': return makeToken(TOKEN_LEFT_BRACKET);
        case ']': return makeToken(TOKEN_RIGHT_BRACKET);
        case ',': return makeToken(TOKEN_COMMA);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case '.': return makeToken(TOKEN_DOT);
        case '+': return makeToken(match('+') ? TOKEN_PLUS_PLUS : match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
        case '-': return makeToken(match('-') ? TOKEN_MINUS_MINUS : match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
        case '*': return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
        case '/': return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        case '%': return makeToken(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
        case '^': return makeToken(match('=') ? TOKEN_CARET_EQUAL : TOKEN_CARET);
        case '?': return makeToken(TOKEN_QMARK);
        case ':': return makeToken(TOKEN_COLON);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : match('<') ? TOKEN_SHIFT_LEFT : TOKEN_LESS);
        case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : match('>') ? TOKEN_SHIFT_RIGHT : TOKEN_GREATER);
        case '"': return string('"');
        case 39 : return string(39); // <- for ' because ASCII(') = 39
        case '$': return makeToken(TOKEN_DOLSIGN);
    }


    return errorToken("Unexpected Character.");
}