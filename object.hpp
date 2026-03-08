#pragma once

#include "chunk.hpp"
#include <string>

struct ObjFunction : Obj {
    int arity;
    Chunk chunk;
    ObjString* name;
};

typedef Value (*NativeFn)(int argCount, Value* args);

struct ObjNative : Obj {
    int arity;
    std::string name;
    NativeFn function;
};

ObjString*   copyString(const char* chars, int length);
ObjString*   allocateString(std::string value);
ObjRange*    allocateRange(double left, double right, double step);
ObjFunction* newFunction();
ObjNative*   newNative(NativeFn function, int arity, const char* name);