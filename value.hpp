#pragma once

#include "common.hpp"

enum ValueType {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
};

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NULL_VAL          ((Value){VAL_NULL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

struct ValueArray
{
    int capacity;
    int count;
    Value *values;
};

void initValueArray(ValueArray *array);
void freeValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void printValue(Value value);