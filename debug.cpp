#include <cstdio>
#include "debug.hpp"
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

static int constantInstructionSmall(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int constantInstructionBig(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant_low  = chunk->code[offset + 3];
    uint8_t constant_mid  = chunk->code[offset + 2];
    uint8_t constant_high = chunk->code[offset + 1];
    uint32_t constant = ((constant_high << 16) | (constant_mid << 8) | (constant_low));
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
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
        return constantInstructionSmall("OP_CONSTANT", chunk, offset);
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
    case OP_INCREMENT:
        return simpleInstruction("OP_INCREMENT", offset);
    case OP_DECREMENT:
        return simpleInstruction("OP_DECREMENT", offset);
    case OP_EQUAL:         
        return simpleInstruction("OP_EQUAL", offset);
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
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}