#pragma once

#include "value.hpp"

ObjString* copyString(const char* chars, int length);
ObjString* allocateString(std::string value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}