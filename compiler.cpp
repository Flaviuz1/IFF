#include "scanner.hpp"
#include "compiler.hpp"
#include "common.hpp"
#include "chunk.hpp"
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
    double value = strtod(parser.previous.start, nullptr);
    emitConstant(NUMBER_VAL(value));
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    expression();
    parsePrecedence(PREC_UNARY);

    switch(operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
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
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);        break;
        case TOKEN_BANG_EQUAL:    emitByte(OP_NOT_EQUAL);    break;
        case TOKEN_GREATER:       emitByte(OP_GREATER);      break;
        case TOKEN_GREATER_EQUAL: emitByte(OP_GREATER_EQUAL);break;
        case TOKEN_LESS:          emitByte(OP_LESS);         break;
        case TOKEN_LESS_EQUAL:    emitByte(OP_LESS_EQUAL);   break;
        default: return;
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_NULL: emitByte(OP_NULL); break;
        default:
            return;
    }
}

static std::unordered_map<TokenType, ParseRule> rules = {
    // Single-character tokens
    {TOKEN_LEFT_PAREN,    {grouping, nullptr, PREC_NONE}},
    {TOKEN_RIGHT_PAREN,   {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_LEFT_BRACKET,  {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_RIGHT_BRACKET, {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_LEFT_BRACE,    {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_RIGHT_BRACE,   {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_COMMA,         {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_DOT,           {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_MINUS,         {unary,    binary,  PREC_TERM}},
    {TOKEN_PLUS,          {nullptr,  binary,  PREC_TERM}},
    {TOKEN_SEMICOLON,     {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_SLASH,         {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_STAR,          {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_CARET,         {nullptr,  binary,  PREC_POWER}},
    {TOKEN_PERCENT,       {nullptr,  binary,  PREC_FACTOR}},
    {TOKEN_QMARK,         {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_COLON,         {nullptr,  nullptr, PREC_NONE}},
    {TOKEN_DOLSIGN,       {nullptr,  nullptr, PREC_NONE}},
    // One or two character tokens
    {TOKEN_PLUS_PLUS,         {nullptr, nullptr, PREC_NONE}},
    {TOKEN_MINUS_MINUS,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_PLUS_EQUAL,        {nullptr, nullptr, PREC_NONE}},
    {TOKEN_MINUS_EQUAL,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_STAR_EQUAL,        {nullptr, nullptr, PREC_NONE}},
    {TOKEN_SLASH_EQUAL,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_CARET_EQUAL,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_PERCENT_EQUAL,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_BANG,              {unary,   nullptr, PREC_NONE}},
    {TOKEN_BANG_EQUAL,        {nullptr, binary,  PREC_EQUALITY}},
    {TOKEN_EQUAL,             {nullptr, nullptr, PREC_NONE}},
    {TOKEN_EQUAL_EQUAL,       {nullptr, binary,  PREC_EQUALITY}},
    {TOKEN_GREATER,           {nullptr, binary,  PREC_COMPARISON}},
    {TOKEN_GREATER_EQUAL,     {nullptr, binary,  PREC_COMPARISON}},
    {TOKEN_LESS,              {nullptr, binary,  PREC_COMPARISON}},
    {TOKEN_LESS_EQUAL,        {nullptr, binary,  PREC_COMPARISON}},
    {TOKEN_SHIFT_LEFT,        {nullptr, binary,  PREC_SHIFT}},
    {TOKEN_SHIFT_RIGHT,       {nullptr, binary,  PREC_SHIFT}},
    {TOKEN_INTERP_START,      {nullptr, nullptr, PREC_NONE}},
    {TOKEN_INTERP_END,        {nullptr, nullptr, PREC_NONE}},
    // Three character tokens
    {TOKEN_SHIFT_LEFT_EQUAL,  {nullptr, nullptr, PREC_NONE}},
    {TOKEN_SHIFT_RIGHT_EQUAL, {nullptr, nullptr, PREC_NONE}},
    // Literals
    {TOKEN_IDENTIFIER, {nullptr, nullptr, PREC_NONE}},
    {TOKEN_STRING,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_NUMBER,     {number,  nullptr, PREC_NONE}},
    {TOKEN_BINARY,     {number,  nullptr, PREC_NONE}},
    {TOKEN_HEX,        {number,  nullptr, PREC_NONE}},
    {TOKEN_OCTAL,      {number,  nullptr, PREC_NONE}},
    // Keywords
    {TOKEN_AND,      {nullptr, nullptr, PREC_NONE}},
    {TOKEN_CLASS,    {nullptr, nullptr, PREC_NONE}},
    {TOKEN_ELSE,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_FALSE,    {literal, nullptr, PREC_NONE}},
    {TOKEN_FOR,      {nullptr, nullptr, PREC_NONE}},
    {TOKEN_FUNC,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_IF,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_NULL,     {literal, nullptr, PREC_NONE}},
    {TOKEN_OR,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_RETURN,   {nullptr, nullptr, PREC_NONE}},
    {TOKEN_SUPER,    {nullptr, nullptr, PREC_NONE}},
    {TOKEN_SELF,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_TRUE,     {literal, nullptr, PREC_NONE}},
    {TOKEN_VAR,      {nullptr, nullptr, PREC_NONE}},
    {TOKEN_WHILE,    {nullptr, nullptr, PREC_NONE}},
    {TOKEN_IN,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_IS,       {nullptr, nullptr, PREC_NONE}},
    {TOKEN_BREAK,    {nullptr, nullptr, PREC_NONE}},
    {TOKEN_CONTINUE, {nullptr, nullptr, PREC_NONE}},
    {TOKEN_MATCH,    {nullptr, nullptr, PREC_NONE}},
    {TOKEN_CASE,     {nullptr, nullptr, PREC_NONE}},
    {TOKEN_IMPORT,   {nullptr, nullptr, PREC_NONE}},
    // Special
    {TOKEN_ERROR, {nullptr, nullptr, PREC_NONE}},
    {TOKEN_EOF,   {nullptr, nullptr, PREC_NONE}},
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

bool compile(const char* source, Chunk* chunk){
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}