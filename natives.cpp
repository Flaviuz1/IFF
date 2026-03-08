#include "natives.hpp"
#include <iostream>
#include <cmath>
#include <ctime>

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

//arrays

//strings