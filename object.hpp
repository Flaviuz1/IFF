#pragma once

#include "chunk.hpp"
#include <string>

struct ObjFunction : Obj {
    int arity;
    Chunk chunk;
    ObjString* name;
};

ObjString*   copyString(const char* chars, int length);
ObjString*   allocateString(std::string value);
ObjRange*    allocateRange(double left, double right, double step);
ObjFunction* newFunction();