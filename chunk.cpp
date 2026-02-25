#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp"

void initChunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = nullptr;
    chunk->lines = nullptr;
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

void writeConstant(Chunk *chunk, Value value, int line)
{
    int constantIndex = addConstant(chunk, value);
    if (constantIndex > 0xFF)
    {
        writeChunk(chunk, OP_CONSTANT_BIG, line);
        writeChunk(chunk, ((constantIndex >> 16) & 0xFF), line);
        writeChunk(chunk, ((constantIndex >> 8) & 0xFF), line);
        writeChunk(chunk, (constantIndex & 0xFF), line);
    }
    else
    {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, constantIndex, line);
    }
}

void freeChunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

int addConstant(Chunk *chunk, Value value)
{
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}