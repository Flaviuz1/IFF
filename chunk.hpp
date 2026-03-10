#pragma once

#include "common.hpp"
#include "value.hpp"

enum OpCode
{
        OP_RETURN,           // 0
        OP_CONSTANT,         // 1
        OP_CONSTANT_BIG,     // 2
        OP_NULL,             // 3
        OP_TRUE,             // 4
        OP_FALSE,            // 5
        OP_NEGATE,           // 6
        OP_ADD,              // 7
        OP_SUBTRACT,         // 8
        OP_MULTIPLY,         // 9
        OP_DIVIDE,           // 10
        OP_MODULO,           // 11
        OP_POWER,            // 12
        OP_INCREMENT,        // 13
        OP_DECREMENT,        // 14
        OP_SHIFT_LEFT,       // 15
        OP_SHIFT_RIGHT,      // 16
        OP_BITWISE_AND,      // 17
        OP_BITWISE_OR,       // 18
        OP_BITWISE_XOR,      // 19
        OP_BITWISE_NOT,      // 20
        OP_EQUAL_EQUAL,      // 21
        OP_BANG_EQUAL,       // 22
        OP_GREATER,          // 23
        OP_GREATER_EQUAL,    // 24
        OP_LESS,             // 25
        OP_LESS_EQUAL,       // 26
        OP_IS,               // 27
        OP_NOT,              // 28
        OP_AND,              // 29
        OP_OR,               // 30
        OP_DEFINE_GLOBAL,    // 31
        OP_SET_GLOBAL,       // 32
        OP_GET_GLOBAL,       // 33
        OP_DEFINE_GLOBAL_BIG,// 34
        OP_SET_GLOBAL_BIG,   // 35
        OP_GET_GLOBAL_BIG,   // 36
        OP_GET_LOCAL,        // 37
        OP_GET_LOCAL_BIG,    // 38
        OP_SET_LOCAL,        // 39
        OP_SET_LOCAL_BIG,    // 40
        OP_CONST,            // 41
        OP_NOT_CONST,        // 42
        OP_JUMP_IF_FALSE,    // 43
        OP_JUMP,             // 44
        OP_RANGE,            // 45
        OP_BY,               // 46
        OP_FOR_LOOP,         // 47
        OP_CALL,             // 48
        OP_POP,              // 49
        OP_DUP,              // 50
        OP_STRINGIFY,        // 51
        OP_LOOP,             // 52
};

struct Chunk
{
    int count;
    int capacity;
    uint8_t *code;
    int *lines;
    ValueArray constants;
};

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
void writeConstant(Chunk *chunk, Value value, int line);