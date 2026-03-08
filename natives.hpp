#pragma once

#include "value.hpp"
#include "object.hpp"

//io
Value clockNative (int argCount, Value* args);
Value printNative (int argCount, Value* args);
Value printnNative(int argCount, Value* args);
Value inputNative (int argCount, Value* args);
//math
Value absNative   (int argCount, Value* args);
//arrays
//strings