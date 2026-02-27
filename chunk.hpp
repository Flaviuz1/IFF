#pragma once

#include "common.hpp"
#include "value.hpp"

enum OpCode
{
    OP_RETURN,
    OP_CONSTANT,
    OP_CONSTANT_BIG,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_RAISETOPOWER
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