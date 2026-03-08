#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "object.hpp"
#include <unordered_map>
#define FRAME_MAX 64
#define UINT8_COUNT (UINT8_MAX + 1)
#define STACK_MAX (FRAME_MAX * UINT8_COUNT)


struct CallFrame {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
};

struct varAtt {
    Value value;
    bool isConst;
};  

struct VM {
    CallFrame frames[FRAME_MAX];
    int frameCount;
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