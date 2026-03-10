#pragma once

#include "common.hpp"
#include <string>
#include <cstring>

struct ObjFunction;

enum ObjType {
    OBJ_STRING,
    OBJ_RANGE,
    OBJ_FUNCTION,
    OBJ_NATIVE
};

struct Obj {
    ObjType type;
    Obj* next;
};

struct ObjRange : Obj {
    double left, right, current, step;
    int dir, iterations;
};

struct ObjString : Obj{
    std::string stringValue;
};

typedef uint64_t Value;
static inline uint64_t doubleToU64(double d) { uint64_t u; memcpy(&u, &d, 8); return u; }
static inline double   u64ToDouble(uint64_t u) { double d; memcpy(&d, &u, 8); return d; }

#define QNAN     ((uint64_t)0x7FFC000000000000)
#define TAG_NULL  1
#define TAG_FALSE 2
#define TAG_TRUE  3

//numbers
#define IS_NUMBER(v)  (((v) & QNAN) != QNAN)
#define AS_NUMBER(v)  (u64ToDouble(v))
#define NUMBER_VAL(n) (doubleToU64((double)(n)))

//booleans
#define NULL_VAL      ((Value)(QNAN | TAG_NULL))
#define FALSE_VAL     ((Value)(QNAN | TAG_FALSE))
#define TRUE_VAL      ((Value)(QNAN | TAG_TRUE))
#define BOOL_VAL(v)   ((v)? TRUE_VAL : FALSE_VAL)
#define IS_NULL(v)    ((v) == NULL_VAL)
#define IS_BOOL(v)    (((v) & FALSE_VAL) == FALSE_VAL)
#define AS_BOOL(v)    ((v) == TRUE_VAL)

//objects (pointers)
#define SIGN_BIT          ((uint64_t)0x8000000000000000)
#define AS_OBJ(v)         ((Obj*)(uintptr_t)((v) & ~(SIGN_BIT | QNAN)))
#define AS_OBJ_TYPE(v, Type) ((Type*)AS_OBJ(v))
#define IS_OBJ(v)         (((v) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define IS_OBJ_TYPE(v, Type) (IS_OBJ(v) && AS_OBJ(v)->type == Type)
#define OBJ_VAL(obj)      ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)))
#define AS_CSTRING(v)     (AS_OBJ_TYPE(v, ObjString)->stringValue.c_str())

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