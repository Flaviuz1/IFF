#pragma once

#include "common.hpp"

enum ValueType {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_OBJ
};

enum ObjType {

};

struct Obj {
    ObjType type;
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        std::string* string;
        Obj* obj;
    } as;
};

struct ObjString : Obj{
    std::string value;
};

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_STRING(value)  ((value).type == VAL_STRING)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_STRING(value)  ((value).as.string)
#define AS_CSTRING(value) ((value).as.string->c_str())
#define AS_OBJ(value)    ((value).as.obj)

#define BOOL_VAL(value)   ((Value){VAL_BOOL,   {.boolean = value}})
#define NULL_VAL          ((Value){VAL_NULL,   {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define STRING_VAL(value) ((Value){VAL_STRING, {.string = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ,    {.obj = (Obj*)object}})

#define VALUE_TO_STRING(value) \
    (IS_STRING(value) ? *AS_STRING(value) : \
     IS_NUMBER(value) ? std::to_string(AS_NUMBER(value)) : \
     IS_BOOL(value)   ? (AS_BOOL(value) ? "true" : "false") : \
     IS_NULL(value)   ? "null" : "")

struct ValueArray
{
    int capacity;
    int count;
    Value *values;
};

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray *array);
void freeValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void printValue(Value value);