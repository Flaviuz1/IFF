#include <cstdio>
#include <cstring>
#include "debug.hpp"
#include "object.hpp"
#include "value.hpp"

void disassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int simpleInstruction(const char *name, int offset)
{
    printf(" %s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t idx = chunk->code[offset + 1];
    printf("%-24s %4d '", name, idx);
    printValue(chunk->constants.values[idx]);
    printf("'\n");
    return offset + 2;
}

static int constantInstructionBig(const char* name, Chunk* chunk, int offset) {
    uint32_t idx = ((uint32_t)chunk->code[offset + 1] << 16)
                 | ((uint32_t)chunk->code[offset + 2] << 8)
                 |  (uint32_t)chunk->code[offset + 3];
    printf("%-24s %4d '", name, idx);
    printValue(chunk->constants.values[idx]);
    printf("'\n");
    return offset + 4;
}

static int defineGlobalInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t idx     = chunk->code[offset + 1];
    bool    isConst = (chunk->code[offset + 2] == OP_CONST);
    printf("%-24s %4d '%s' (%s)\n", name, idx,
        AS_CSTRING(chunk->constants.values[idx]),
        isConst ? "const" : "var");
    return offset + 3;
}

static int defineGlobalBigInstruction(const char* name, Chunk* chunk, int offset) {
    uint32_t idx = ((uint32_t)chunk->code[offset + 1] << 16)
                 | ((uint32_t)chunk->code[offset + 2] << 8)
                 |  (uint32_t)chunk->code[offset + 3];
    bool isConst = (chunk->code[offset + 4] == OP_CONST);
    printf("%-24s %4d '%s' (%s)\n", name, idx,
        AS_CSTRING(chunk->constants.values[idx]),
        isConst ? "const" : "var");
    return offset + 5;
}

static int localInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-24s %4d\n", name, slot);
    return offset + 2;
}

static int localInstructionBig(const char* name, Chunk* chunk, int offset) {
    uint32_t slot = ((uint32_t)chunk->code[offset + 1] << 16)
                  | ((uint32_t)chunk->code[offset + 2] << 8)
                  |  (uint32_t)chunk->code[offset + 3];
    printf("%-24s %4d\n", name, slot);
    return offset + 4;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint32_t jump = ((uint32_t)chunk->code[offset + 1] << 16)
                  | ((uint32_t)chunk->code[offset + 2] << 8)
                  |  (uint32_t)chunk->code[offset + 3];
    printf("%-16s %4d -> %d\n", name, offset, offset + 4 + sign * jump);
    return offset + 4;
}

int disassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        printf(" | ");
    }
    else
    {
        printf("%4d ", chunk->lines[offset]);
    }
    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_BIG:
        return constantInstructionBig("OP_CONSTANT_BIG", chunk, offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_POWER:
        return simpleInstruction("OP_RAISETOPOWER", offset);
    case OP_NULL:
        return simpleInstruction("OP_NULL", offset);
    case OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
        return simpleInstruction("OP_FALSE", offset);
    case OP_NOT:
        return simpleInstruction("OP_NOT", offset);
    case OP_AND:
        return simpleInstruction("OP_AND", offset);
    case OP_OR:
        return simpleInstruction("OP_OR", offset);
    case OP_INCREMENT:
        return simpleInstruction("OP_INCREMENT", offset);
    case OP_DECREMENT:
        return simpleInstruction("OP_DECREMENT", offset);
    case OP_EQUAL_EQUAL:
        return simpleInstruction("OP_EQUAL_EQUAL", offset);
    case OP_BANG_EQUAL:    
        return simpleInstruction("OP_NOT_EQUAL", offset);
    case OP_GREATER:       
        return simpleInstruction("OP_GREATER",offset);
    case OP_GREATER_EQUAL: 
        return simpleInstruction("OP_GREATER_EQUAL", offset);
    case OP_LESS:          
        return simpleInstruction("OP_LESS", offset);
    case OP_LESS_EQUAL:    
        return simpleInstruction("OP_LESS_EQUAL", offset);
    case OP_MODULO:
        return simpleInstruction("OP_MODULO", offset);
    case OP_SHIFT_LEFT:
        return simpleInstruction("OP_SHIFT_LEFT", offset);
    case OP_SHIFT_RIGHT:
        return simpleInstruction("OP_SHIFT_RIGHT", offset);
    case OP_BITWISE_AND:
        return simpleInstruction("OP_BITWISE_AND", offset);
    case OP_BITWISE_OR:
        return simpleInstruction("OP_BITWISE_OR", offset);
    case OP_BITWISE_XOR:
        return simpleInstruction("OP_BITWISE_XOR", offset);
    case OP_BITWISE_NOT:
        return simpleInstruction("OP_BITWISE_NOT", offset);
    case OP_STRINGIFY:
        return simpleInstruction("OP_STRINGIFY", offset);
    case OP_DEFINE_GLOBAL:
        return defineGlobalInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL_BIG: 
        return defineGlobalBigInstruction("OP_DEFINE_GLOBAL_BIG",  chunk, offset);
    case OP_GET_GLOBAL:        
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:        
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL_BIG:    
        return constantInstructionBig("OP_GET_GLOBAL_BIG", chunk, offset);
    case OP_SET_GLOBAL_BIG:    
        return constantInstructionBig("OP_SET_GLOBAL_BIG", chunk, offset);
    case OP_GET_LOCAL:         
        return localInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:         
        return localInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_LOCAL_BIG:     
        return localInstructionBig("OP_GET_LOCAL_BIG", chunk, offset);
    case OP_SET_LOCAL_BIG:     
        return localInstructionBig("OP_SET_LOCAL_BIG", chunk, offset);
    case OP_POP:
        return simpleInstruction("OP_SEMICOLON / OP_POP", offset);
    case OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_ELSE", 1, chunk, offset);
    case OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_DUP:
        return simpleInstruction("OP_DUP", offset);
    case OP_CALL:
        return constantInstruction("OP_CALL", chunk, offset);
    case OP_RANGE:
        return simpleInstruction("OP_RANGE", offset);
    case OP_BY:
        return simpleInstruction("OP_BY", offset);
    case OP_FOR_ITERATE:
        return jumpInstruction("OP_FOR_ITERATE", 1, chunk, offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}