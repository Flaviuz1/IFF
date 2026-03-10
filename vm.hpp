#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include "object.hpp"
#include <vector>
#define FRAME_MAX 512
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
    std::vector<varAtt> globals;
    std::vector<std::string> globalNames;
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