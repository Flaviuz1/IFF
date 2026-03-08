#include "natives.hpp"
#include <iostream>
#include <cmath>
#include <ctime>
#include <cfloat>

//io
Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value printNative(int argCount, Value* args) {
    printValue(args[0]);
    printf("\n");
    return NULL_VAL;
}

Value printnNative(int argCount, Value* args) {
    printValue(args[0]);
    return NULL_VAL;
}

Value inputNative(int argCount, Value* args) {
    if (argCount > 0) {
        printValue(args[0]);  // optional prompt
    }
    std::string line;
    std::getline(std::cin, line);
    return STRING_VAL(copyString(line.c_str(), line.length()));
}

//math
Value absNative(int argCount, Value* args) {
    return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

Value maxNative(int argCount, Value* args) {
    double MAX = DBL_MIN;
    for(int i = 0; i < argCount; i++) {
        double num = AS_NUMBER(args[i]);
        if (MAX < num) MAX = num;
    }
    return NUMBER_VAL(MAX);
}

Value minNative(int argCount, Value* args) {
    double MIN = DBL_MAX;
    for(int i = 0; i < argCount; i++) {
        double num = AS_NUMBER(args[i]);
        if (MIN > num) MIN = num;
    }
    return NUMBER_VAL(MIN);
}

Value floorNative(int argCount, Value* args) {
    return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

Value ceilNative(int argCount, Value* args) {
    return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

Value roundNative(int argCount, Value* args) {
    return NUMBER_VAL(round(AS_NUMBER(args[0])));
}

Value truncNative(int argCount, Value* args) {
    return NUMBER_VAL(trunc(AS_NUMBER(args[0])));
}

Value signNative(int argCount, Value* args) {
    double n = AS_NUMBER(args[0]);
    return NUMBER_VAL((double)(n > 0 ? 1 : n < 0 ? -1 : 0));
}

Value fractNative(int argCount, Value* args) {
    double n = AS_NUMBER(args[0]);
    return NUMBER_VAL(n - floor(n));
}

Value clampNative(int argCount, Value* args) {
    double val = AS_NUMBER(args[0]);
    double mn  = AS_NUMBER(args[1]);
    double mx  = AS_NUMBER(args[2]);
    return NUMBER_VAL(val < mn ? mn : val > mx ? mx : val);
}

Value lerpNative(int argCount, Value* args) {
    double a = AS_NUMBER(args[0]);
    double b = AS_NUMBER(args[1]);
    double t = AS_NUMBER(args[2]);
    return NUMBER_VAL(a + t * (b - a));
}

Value expNative(int argCount, Value* args) {
    return NUMBER_VAL(exp(AS_NUMBER(args[0])));
}

Value logNative(int argCount, Value* args) {
    return NUMBER_VAL(log(AS_NUMBER(args[0])));
}

Value log2Native(int argCount, Value* args) {
    return NUMBER_VAL(log2(AS_NUMBER(args[0])));
}

Value log10Native(int argCount, Value* args) {
    return NUMBER_VAL(log10(AS_NUMBER(args[0])));
}

Value sinNative(int argCount, Value* args) {
    return NUMBER_VAL(sin(AS_NUMBER(args[0])));
}

Value cosNative(int argCount, Value* args) {
    return NUMBER_VAL(cos(AS_NUMBER(args[0])));
}

Value tanNative(int argCount, Value* args) {
    return NUMBER_VAL(tan(AS_NUMBER(args[0])));
}

Value asinNative(int argCount, Value* args) {
    return NUMBER_VAL(asin(AS_NUMBER(args[0])));
}

Value acosNative(int argCount, Value* args) {
    return NUMBER_VAL(acos(AS_NUMBER(args[0])));
}

Value atanNative(int argCount, Value* args) {
    return NUMBER_VAL(atan(AS_NUMBER(args[0])));
}

Value atan2Native(int argCount, Value* args) {
    return NUMBER_VAL(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

//random
    
//arrays

//strings