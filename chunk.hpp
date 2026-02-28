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
    // Bitwise
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,
    // Comparison
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    // Logical
    OP_NOT,
    OP_AND,
    OP_OR,
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