#pragma once

#include "common.hpp"
#include <string>

enum ValueType {
    VAL_BOOL,
    VAL_NULL,
    VAL_NUMBER,
    VAL_OBJ
};

enum ObjType {
    OBJ_STRING
};

struct Obj {
    ObjType type;
    Obj* next;
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
};

struct ObjString : Obj{
    std::string value;
};

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->value.c_str())

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
#define IS_STRING(value)  (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_STRING)

#define BOOL_VAL(value)   ((Value){VAL_BOOL,   {.boolean = value}})
#define NULL_VAL          ((Value){VAL_NULL,   {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define STRING_VAL(value) ((Value){VAL_OBJ,    {.obj = (Obj*)value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ,    {.obj = (Obj*)object}})

#define OBJ_TYPE(value)   (AS_OBJ(value)->type)

#define VALUE_TO_STRING(_v) valueToString(_v)

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