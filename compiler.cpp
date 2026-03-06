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
    PREC_UNARY,       // ! - ~
    PREC_CALL,        // . ()
    PREC_PRIMARY
};

typedef void (*ParseFn)(bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

static void expression();
static void statement();
static void declaration();
static void variable(bool canAssign);
static void unary(bool canAssign);
static void number(bool canAssign);
static void literal(bool canAssign);
static void string(bool canAssign);
static void grouping(bool canAssign);
static void binary(bool canAssign);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, const char* msg) {
    if (parser.panicMode) return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // nothing extra
    } else {
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

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
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

static void emitByte(uint8_t byte) {
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
            (uint8_t)((constant >> 8)  & 0xFF),
            (uint8_t)( constant        & 0xFF)
        };
    }
    return {OP_CONSTANT, (uint8_t)constant};
}

static void emitConstant(Value value) {
    emitBytes(makeConstant(value));
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
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

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE);      break;
        case TOKEN_BANG:  emitByte(OP_NOT);         break;
        case TOKEN_TILDE: emitByte(OP_BITWISE_NOT); break;
        default: return;
    }
}

static void binary(bool canAssign) {
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

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE:  emitByte(OP_TRUE);  break;
        case TOKEN_NULL:  emitByte(OP_NULL);  break;
        default: return;
    }
}

static void string(bool canAssign) {
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

static void emitGlobalOp(uint8_t smallOp, uint8_t bigOp, int index) {
    if (index > UINT8_MAX) {
        emitBytes({bigOp,
            (uint8_t)((index >> 16) & 0xFF),
            (uint8_t)((index >> 8)  & 0xFF),
            (uint8_t)( index        & 0xFF)
        });
    } else {
        emitBytes({smallOp, (uint8_t)index});
    }
}

static void variable(bool canAssign) {
    int nameConstant = addConstant(currentChunk(), STRING_VAL(
        copyString(parser.previous.start, parser.previous.length)
    ));

    if (canAssign && parser.current.type == TOKEN_EQUAL) {
        advance();
        expression();
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_PLUS_PLUS) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        emitByte(OP_INCREMENT);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_MINUS_MINUS) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        emitByte(OP_DECREMENT);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_PLUS_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_ADD);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_MINUS_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_SUBTRACT);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_STAR_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_MULTIPLY);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_SLASH_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_DIVIDE);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_PERCENT_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_MODULO);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_CARET_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_POWER);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_AMPERSAND_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_BITWISE_AND);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_PIPE_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_BITWISE_OR);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_HASH_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_BITWISE_XOR);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_SHIFT_LEFT_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_SHIFT_LEFT);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else if (canAssign && parser.current.type == TOKEN_SHIFT_RIGHT_EQUAL) {
        advance();
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
        expression();
        emitByte(OP_SHIFT_RIGHT);
        emitGlobalOp(OP_SET_GLOBAL, OP_SET_GLOBAL_BIG, nameConstant);
    } else {
        emitGlobalOp(OP_GET_GLOBAL, OP_GET_GLOBAL_BIG, nameConstant);
    }
}

static std::unordered_map<TokenType, ParseRule> rules = {
    // Single-character tokens
    {TOKEN_LEFT_PAREN,        {grouping, nullptr/*call*/,    PREC_CALL}},
    {TOKEN_RIGHT_PAREN,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_LEFT_BRACKET,      {nullptr,  nullptr/*index*/,   PREC_CALL}},
    {TOKEN_RIGHT_BRACKET,     {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_LEFT_BRACE,        {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_RIGHT_BRACE,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_COMMA,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_DOT,               {nullptr,  nullptr/*dot*/,     PREC_CALL}},
    {TOKEN_MINUS,             {unary,    binary,             PREC_TERM}},
    {TOKEN_PLUS,              {nullptr,  binary,             PREC_TERM}},
    {TOKEN_SEMICOLON,         {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_SLASH,             {nullptr,  binary,             PREC_FACTOR}},
    {TOKEN_STAR,              {nullptr,  binary,             PREC_FACTOR}},
    {TOKEN_CARET,             {nullptr,  binary,             PREC_POWER}},
    {TOKEN_PERCENT,           {nullptr,  binary,             PREC_FACTOR}},
    {TOKEN_QMARK,             {nullptr,  nullptr/*ternary*/, PREC_TERNARY}},
    {TOKEN_COLON,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_DOLSIGN,           {nullptr,  nullptr,            PREC_NONE}},
    // One or two character tokens
    {TOKEN_PLUS_PLUS,         {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_MINUS_MINUS,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_PLUS_EQUAL,        {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_MINUS_EQUAL,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_STAR_EQUAL,        {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_SLASH_EQUAL,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_CARET_EQUAL,       {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_PERCENT_EQUAL,     {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_BANG,              {unary,    nullptr,            PREC_NONE}},
    {TOKEN_BANG_EQUAL,        {nullptr,  binary,             PREC_EQUALITY}},
    {TOKEN_EQUAL,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_EQUAL_EQUAL,       {nullptr,  binary,             PREC_EQUALITY}},
    {TOKEN_GREATER,           {nullptr,  binary,             PREC_COMPARISON}},
    {TOKEN_GREATER_EQUAL,     {nullptr,  binary,             PREC_COMPARISON}},
    {TOKEN_LESS,              {nullptr,  binary,             PREC_COMPARISON}},
    {TOKEN_LESS_EQUAL,        {nullptr,  binary,             PREC_COMPARISON}},
    {TOKEN_SHIFT_LEFT,        {nullptr,  binary,             PREC_SHIFT}},
    {TOKEN_SHIFT_RIGHT,       {nullptr,  binary,             PREC_SHIFT}},
    {TOKEN_AMPERSAND,         {nullptr,  binary,             PREC_BITWISE_AND}},
    {TOKEN_HASH,              {nullptr,  binary,             PREC_BITWISE_XOR}},
    {TOKEN_PIPE,              {nullptr,  binary,             PREC_BITWISE_OR}},
    {TOKEN_TILDE,             {unary,    nullptr,            PREC_NONE}},
    {TOKEN_INTERP_START,      {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_INTERP_END,        {nullptr,  nullptr,            PREC_NONE}},
    // Three character tokens
    {TOKEN_SHIFT_LEFT_EQUAL,  {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_SHIFT_RIGHT_EQUAL, {nullptr,  nullptr,            PREC_NONE}},
    // Literals
    {TOKEN_IDENTIFIER,        {variable, nullptr,            PREC_NONE}},
    {TOKEN_STRING,            {string,   nullptr,            PREC_NONE}},
    {TOKEN_NUMBER,            {number,   nullptr,            PREC_NONE}},
    {TOKEN_BINARY,            {number,   nullptr,            PREC_NONE}},
    {TOKEN_HEX,               {number,   nullptr,            PREC_NONE}},
    {TOKEN_OCTAL,             {number,   nullptr,            PREC_NONE}},
    // Keywords
    {TOKEN_AND,               {nullptr,  nullptr/*and_*/,    PREC_AND}},
    {TOKEN_CLASS,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_ELSE,              {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_FALSE,             {literal,  nullptr,            PREC_NONE}},
    {TOKEN_FOR,               {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_FUNC,              {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_IF,                {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_NULL,              {literal,  nullptr,            PREC_NONE}},
    {TOKEN_OR,                {nullptr,  nullptr/*or_*/,     PREC_OR}},
    {TOKEN_RETURN,            {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_SUPER,             {nullptr/*super*/,  nullptr,   PREC_NONE}},
    {TOKEN_SELF,              {nullptr/*self*/,   nullptr,   PREC_NONE}},
    {TOKEN_TRUE,              {literal,  nullptr,            PREC_NONE}},
    {TOKEN_VAR,               {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_PRINT_PLACEHOLDER, {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_WHILE,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_IN,                {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_IS,                {nullptr,  binary,             PREC_COMPARISON}},
    {TOKEN_BREAK,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_CONTINUE,          {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_MATCH,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_CASE,              {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_IMPORT,            {nullptr,  nullptr,            PREC_NONE}},
    // Special
    {TOKEN_ERROR,             {nullptr,  nullptr,            PREC_NONE}},
    {TOKEN_EOF,               {nullptr,  nullptr,            PREC_NONE}},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression.");
        return;
    }

    bool canAssign = (precedence <= PREC_ASSIGNMENT);
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign == false && parser.current.type == TOKEN_EQUAL) {
        error("Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void varDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect variable name.");
    uint8_t nameConstant = addConstant(currentChunk(), STRING_VAL(
        copyString(parser.previous.start, parser.previous.length)
    ));

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
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
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
                return;
            default:
                break;
        }
        advance();
    }
}

static bool check(TokenType type) {
    return (parser.current.type == type);
}

static bool match(TokenType type) {
    if (!check(type)) return false;
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

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    endCompiler();
    return !parser.hadError;
}