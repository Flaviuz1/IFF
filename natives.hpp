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
Value randomNative(int argCount, Value* args);
Value seedNative  (int argCount, Value* args);
Value randominNative(int argCount, Value* args);
//arrays

//strings
Value lenNative         (int argCount, Value* args);
Value upperNative       (int argCount, Value* args);
Value lowerNative       (int argCount, Value* args);
Value trimNative        (int argCount, Value* args);
Value substrNative      (int argCount, Value* args);
Value containsNative    (int argCount, Value* args);
Value indexOfNative     (int argCount, Value* args);
Value charAtNative      (int argCount, Value* args);
Value charCodeNative    (int argCount, Value* args);
Value fromCharCodeNative(int argCount, Value* args);
Value replaceNative     (int argCount, Value* args);
Value repeatNative      (int argCount, Value* args);
Value startsWithNative  (int argCount, Value* args);
Value endsWithNative    (int argCount, Value* args);

//conversions
Value toNumberNative     (int argCount, Value* args);
Value toStringNative     (int argCount, Value* args);
Value toBoolNative       (int argCount, Value* args);
Value typeOfNative       (int argCount, Value* args);
Value isNumberNative     (int argCount, Value* args);
Value isStringNative     (int argCount, Value* args);
Value isBoolNative       (int argCount, Value* args);
Value isNullNative       (int argCount, Value* args);
Value isFuncNative       (int argCount, Value* args);
Value isNaNNative        (int argCount, Value* args);
Value isInfNative        (int argCount, Value* args);