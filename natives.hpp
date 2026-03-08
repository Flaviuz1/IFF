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
Value maxNative   (int argCount, Value* args);
Value minNative   (int argCount, Value* args);
Value floorNative (int argCount, Value* args);
Value ceilNative  (int argCount, Value* args);
Value roundNative (int argCount, Value* args);
Value truncNative (int argCount, Value* args);
Value signNative  (int argCount, Value* args);
Value fractNative (int argCount, Value* args);
Value clampNative (int argCount, Value* args);
Value lerpNative  (int argCount, Value* args);
Value expNative   (int argCount, Value* args);
Value logNative   (int argCount, Value* args);
Value log2Native  (int argCount, Value* args);
Value log10Native (int argCount, Value* args);
Value sinNative   (int argCount, Value* args);
Value cosNative   (int argCount, Value* args);
Value tanNative   (int argCount, Value* args);
Value asinNative  (int argCount, Value* args);
Value acosNative  (int argCount, Value* args);
Value atanNative  (int argCount, Value* args);
Value atan2Native (int argCount, Value* args);
//random    
//arrays
//strings