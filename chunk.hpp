#pragma once

#include "common.hpp"
#include "value.hpp"

enum OpCode
{
    OP_RETURN,
    // Constants
    OP_CONSTANT,
    OP_CONSTANT_BIG,
    // Literals
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    // Arithmetic
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_POWER,
    OP_INCREMENT,
    OP_DECREMENT,
    // Bitwise
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_NOT,
    // Comparison
    OP_EQUAL_EQUAL,
    OP_BANG_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_IS,
    // Logical
    OP_NOT,
    OP_AND,
    OP_OR,
    // Variables and so
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL_BIG,
    OP_SET_GLOBAL_BIG,
    OP_GET_GLOBAL_BIG,
    OP_GET_LOCAL,
    OP_GET_LOCAL_BIG,
    OP_SET_LOCAL,
    OP_SET_LOCAL_BIG,
    OP_CONST, OP_NOT_CONST,
    // Jumps
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    // For loops
    OP_RANGE,
    OP_BY,
    OP_FOR_ITERATE,
    // Funcions
    OP_CALL,
    // Others
    OP_POP,
    OP_DUP,
    OP_STRINGIFY
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