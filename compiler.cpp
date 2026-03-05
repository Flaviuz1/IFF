#include "scanner.hpp"
#include "compiler.hpp"
#include "common.hpp"
#include "chunk.hpp"
#include "object.hpp"
#ifdef DEBUG_PRINT_CODE
    #include "debug.hpp"
#endif
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

struct Parser {
    Token previous;
    Token current;
    bool hadError;
    bool panicMode;
};

enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,  // = += -= *= /= %= ^= <<= >>=
    PREC_TERNARY,     // ?:
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_BITWISE_OR,  // |
    PREC_BITWISE_XOR, // #
    PREC_BITWISE_AND, // &
    PREC_COMPARISON,  // < > <= >=
    PREC_SHIFT,       // << >>
    PREC_TERM,        // + -
    PREC_FACTOR,      // * / %
    PREC_POWER,       // ^
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
};

typedef void (*ParseFn)();

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

static void expression();
static void statement();
static void declaration();
static void variable();
static void unary();
static void number();
static void literal();
static void string();
static void grouping();
static void binary();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk(){
    return compilingChunk;
}

static void errorAt(Token* token, const char* msg) {
    if(parser.panicMode) return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if(token->type == TOKEN_EOF){
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR){
        //smh
    }
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.hadError = true;
}

static void error(const char* msg) {
    errorAt(&parser.current, msg);
}

static void advance() {
    parser.previous = parser.current;

    for(;;) {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR) break;
        error(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    error(message);
}

static void emitByte(uint8_t byte){
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(std::vector<uint8_t> bytes) {
    for (auto byte : bytes) {
        emitByte(byte);
    }
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static void endCompiler() {
    emitReturn();
    #ifdef DEBUG_PRINT_CODE
        if (!parser.hadError) {
            disassembleChunk(currentChunk(), "code");
        }
    #endif
}

static std::vector<uint8_t> makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        return {
            OP_CONSTANT_BIG,
            (uint8_t)((constant >> 16) & 0xFF),
            (uint8_t)((constant >> 8) & 0xFF),
            (uint8_t)(constant & 0xFF)
        };
    }
    return {OP_CONSTANT, (uint8_t)constant};
}

static void emitConstant(Value value){
    emitBytes(makeConstant(value));
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
    double value;
    switch (parser.previous.type) {
        case TOKEN_HEX:
            value = (double)strtol(parser.previous.start + 2, nullptr, 16);
            break;
        case TOKEN_OCTAL:
            value = (double)strtol(parser.previous.start + 2, nullptr, 8);
            break;
        case TOKEN_BINARY:
            value = (double)strtol(parser.previous.start + 2, nullptr, 2);
            break;
        default:
            value = strtod(parser.previous.start, nullptr);
            break;
    }
    emitConstant(NUMBER_VAL(value));
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch(operatorType) {
        case TOKEN_MINUS:         emitByte(OP_NEGATE);       break;
        case TOKEN_BANG:          emitByte(OP_NOT);          break;
        case TOKEN_TILDE:         emitByte(OP_BITWISE_NOT);  break;
        default:
            return;
    }
}

static void binary() {
    TokenType operatorType = parser.previous.type;

    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:          emitByte(OP_ADD);          break;
        case TOKEN_MINUS:         emitByte(OP_SUBTRACT);     break;
        case TOKEN_STAR:          emitByte(OP_MULTIPLY);     break;
        case TOKEN_SLASH:         emitByte(OP_DIVIDE);       break;
        case TOKEN_PERCENT:       emitByte(OP_MODULO);       break;
        case TOKEN_CARET:         emitByte(OP_POWER);        break;
        case TOKEN_SHIFT_LEFT:    emitByte(OP_SHIFT_LEFT);   break;
        case TOKEN_SHIFT_RIGHT:   emitByte(OP_SHIFT_RIGHT);  break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL_EQUAL);  break;
        case TOKEN_BANG_EQUAL:    emitByte(OP_BANG_EQUAL);   break;
        case TOKEN_GREATER:       emitByte(OP_GREATER);      break;
        case TOKEN_GREATER_EQUAL: emitByte(OP_GREATER_EQUAL);break;
        case TOKEN_LESS:          emitByte(OP_LESS);         break;
        case TOKEN_LESS_EQUAL:    emitByte(OP_LESS_EQUAL);   break;
        case TOKEN_AMPERSAND:     emitByte(OP_BITWISE_AND);  break;
        case TOKEN_PIPE:          emitByte(OP_BITWISE_OR);   break;
        case TOKEN_HASH:          emitByte(OP_BITWISE_XOR);  break;
        default: return;
    }
}

static void literal() {
    TokenType operatorType = parser.previous.type;

    switch (operatorType) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE:  emitByte(OP_TRUE); break;
        case TOKEN_NULL:  emitByte(OP_NULL); break;
        default:
            return;
    }
}

static void string() {
    int len = parser.previous.length;
    if (parser.current.type != TOKEN_INTERP_START) len--;
    emitConstant(STRING_VAL(copyString(parser.previous.start, len)));
    
    while (parser.current.type == TOKEN_INTERP_START) {
        advance();
        expression();
        emitByte(OP_STRINGIFY);
        emitByte(OP_ADD);
        consume(TOKEN_INTERP_END, "Expect '}' after interpolation.");
        
        if (parser.current.type == TOKEN_STRING) {
            advance();
            int segLen = parser.previous.length;
            if (parser.current.type != TOKEN_INTERP_START) segLen--;
            emitConstant(STRING_VAL(copyString(parser.previous.start, segLen)));
            emitByte(OP_ADD);
        }
    }
}

static void variable() {
    uint8_t nameConstant = addConstant(currentChunk(), STRING_VAL(
        copyString(parser.previous.start, parser.previous.length)
    ));

    if        (parser.current.type == TOKEN_EQUAL) {
        advance();
        expression();
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_PLUS_PLUS) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        emitByte(OP_INCREMENT);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_MINUS_MINUS) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        emitByte(OP_DECREMENT);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_PLUS_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_ADD);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_MINUS_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_SUBTRACT);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_STAR_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_MULTIPLY);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_SLASH_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_DIVIDE);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_PERCENT_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_MODULO);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_CARET_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_POWER);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_AMPERSAND_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_BITWISE_AND);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_PIPE_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_BITWISE_OR);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_HASH_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_BITWISE_XOR);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_SHIFT_LEFT_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_SHIFT_LEFT);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else if (parser.current.type == TOKEN_SHIFT_RIGHT_EQUAL) {
        advance();
        emitBytes({OP_GET_GLOBAL, nameConstant});
        expression();
        emitByte(OP_SHIFT_RIGHT);
        emitBytes({OP_SET_GLOBAL, nameConstant});
    } else {
        emitBytes({OP_GET_GLOBAL, nameConstant});
    }
}

static std::unordered_map<TokenType, ParseRule> rules = {
    // Single-character tokens
    {TOKEN_LEFT_PAREN,        {grouping, nullptr/*call*/,    PREC_CALL}},
    {TOKEN_RIGHT_PAREN,       {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_LEFT_BRACKET,      {nullptr,  nullptr/*index*/,   PREC_CALL}},
    {TOKEN_RIGHT_BRACKET,     {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_LEFT_BRACE,        {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_RIGHT_BRACE,       {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_COMMA,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_DOT,               {nullptr,  nullptr/*dot*/,     PREC_CALL}},
    {TOKEN_MINUS,             {unary,    binary,  PREC_TERM}},
    {TOKEN_PLUS,              {nullptr,  binary,  PREC_TERM}},
    {TOKEN_SEMICOLON,         {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_SLASH,             {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_STAR,              {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_CARET,             {nullptr,  binary,  PREC_POWER}},
    {TOKEN_PERCENT,           {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_QMARK,             {nullptr,  nullptr/*ternary*/, PREC_TERNARY}},
    {TOKEN_COLON,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_DOLSIGN,           {nullptr,  nullptr, PREC_NONE}},
    // One or two character tokens
    {TOKEN_PLUS_PLUS,         {nullptr,  nullptr, PREC_CALL}},
    {TOKEN_MINUS_MINUS,       {nullptr,  nullptr, PREC_CALL}},
    {TOKEN_PLUS_EQUAL,        {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_MINUS_EQUAL,       {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_STAR_EQUAL,        {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_SLASH_EQUAL,       {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_CARET_EQUAL,       {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_PERCENT_EQUAL,     {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_BANG,              {unary,    nullptr, PREC_NONE}},
    {TOKEN_BANG_EQUAL,        {nullptr,  binary,  PREC_EQUALITY}},
    {TOKEN_EQUAL,             {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_EQUAL_EQUAL,       {nullptr,  binary,  PREC_EQUALITY}},
    {TOKEN_GREATER,           {nullptr,  binary,  PREC_COMPARISON}},
    {TOKEN_GREATER_EQUAL,     {nullptr,  binary,  PREC_COMPARISON}},
    {TOKEN_LESS,              {nullptr,  binary,  PREC_COMPARISON}},
    {TOKEN_LESS_EQUAL,        {nullptr,  binary,  PREC_COMPARISON}},
    {TOKEN_SHIFT_LEFT,        {nullptr,  binary,  PREC_SHIFT}},
    {TOKEN_SHIFT_RIGHT,       {nullptr,  binary,  PREC_SHIFT}},
    {TOKEN_AMPERSAND,         {nullptr,  binary,  PREC_BITWISE_AND}},
    {TOKEN_HASH,              {nullptr,  binary,  PREC_BITWISE_XOR}},
    {TOKEN_PIPE,              {nullptr,  binary,  PREC_BITWISE_OR}},
    {TOKEN_TILDE,             {unary,    nullptr, PREC_NONE}},
    {TOKEN_INTERP_START,      {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_INTERP_END,        {nullptr,  nullptr, PREC_NONE}},
    // Three character tokens
    {TOKEN_SHIFT_LEFT_EQUAL,  {nullptr,  binary,  PREC_ASSIGNMENT}},
    {TOKEN_SHIFT_RIGHT_EQUAL, {nullptr,  binary,  PREC_ASSIGNMENT}},
    // Literals
    {TOKEN_IDENTIFIER,        {variable, nullptr, PREC_NONE}},
    {TOKEN_STRING,            {string,   nullptr, PREC_NONE}},
    {TOKEN_NUMBER,            {number,   nullptr, PREC_NONE}},
    {TOKEN_BINARY,            {number,   nullptr, PREC_NONE}},
    {TOKEN_HEX,               {number,   nullptr, PREC_NONE}},
    {TOKEN_OCTAL,             {number,   nullptr, PREC_NONE}},
    // Keywords
    {TOKEN_AND,               {nullptr,  nullptr/*and_*/,    PREC_AND}},
    {TOKEN_CLASS,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_ELSE,              {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_FALSE,             {literal,  nullptr, PREC_NONE}},
    {TOKEN_FOR,               {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_FUNC,              {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_IF,                {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_NULL,              {literal,  nullptr, PREC_NONE}},
    {TOKEN_OR,                {nullptr,  nullptr/*or_*/,     PREC_OR}},
    {TOKEN_RETURN,            {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_SUPER,             {nullptr/*super*/,    nullptr, PREC_NONE}},
    {TOKEN_SELF,              {nullptr/*self*/,     nullptr, PREC_NONE}},
    {TOKEN_TRUE,              {literal,  nullptr, PREC_NONE}},
    {TOKEN_VAR,               {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_PRINT_PLACEHOLDER, {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_WHILE,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_IN,                {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_IS,                {nullptr,  binary,  PREC_COMPARISON}},
    {TOKEN_BREAK,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_CONTINUE,          {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_MATCH,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_CASE,              {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_IMPORT,            {nullptr,  nullptr, PREC_NONE}},
    // Special
    {TOKEN_ERROR,             {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_EOF,               {nullptr,  nullptr, PREC_NONE}},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression.");
        return;
    }
    prefixRule();
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// STATEMENTS AND DECLARATIONS

static void varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect variable name.");
    uint8_t nameConstant = addConstant(currentChunk(), STRING_VAL(
        copyString(parser.previous.start, parser.previous.length)
    ));
    
    // compile initializer or default to null
    if (parser.current.type == TOKEN_EQUAL) {
        advance();
        expression();
    } else {
        emitByte(OP_NULL);
    }
    
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration or value.");
    emitBytes({OP_DEFINE_GLOBAL, nameConstant});
}

static void printStatementPlaceholder() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration or value.");
    emitByte(OP_PRINT_PLACEHOLDER);
}

static void synchronize() {
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
                switch (parser.current.type) {
                case TOKEN_CLASS:
                case TOKEN_FUNC:
                case TOKEN_VAR:
                case TOKEN_FOR:
                case TOKEN_IF:
                case TOKEN_WHILE:
                case TOKEN_PRINT_PLACEHOLDER:
                case TOKEN_RETURN:
                default:                 
                return;
                // Do nothing.
            }
        advance();
    }
}

static bool check(TokenType type) {
    return (parser.current.type == type);
}

static bool match(TokenType type) {
    if(!check(type)) return false;
    advance();
    return true;
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void statement() {
    if (match(TOKEN_PRINT_PLACEHOLDER)) {
        printStatementPlaceholder();
    } else {
        expressionStatement();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
    
}

bool compile(const char* source, Chunk* chunk){
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    while(!match(TOKEN_EOF)) {
        declaration();
    }
    endCompiler();
    return !parser.hadError;
}