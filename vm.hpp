#pragma once

#include "chunk.hpp"
#include <unordered_map>
#define STACK_MAX 1024

struct varAtt {
    Value value;
    bool isConst;
};  

struct VM{
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
    std::unordered_map<std::string, varAtt> globals;
};

enum InterpretResult{
    INTERPRET_OK, 
    INTERPRET_COMPILE_ERROR, 
    INTERPRET_RUNTIME_ERROR
};

void initVM();
void freeVM(); 
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();