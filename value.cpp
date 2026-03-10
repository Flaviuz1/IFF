#include <cstdio>
#include "memory.hpp"
#include "value.hpp"
#include "object.hpp"

void initValueArray(ValueArray *array)
{
    array->values = nullptr;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

bool valuesEqual(Value a, Value b) {
    if (IS_NUMBER(a) && IS_NUMBER(b))
        return AS_NUMBER(a) == AS_NUMBER(b);
    
    if (!IS_OBJ(a) && !IS_OBJ(b))
        return a == b;
    
    if (!IS_OBJ(a) || !IS_OBJ(b)) return false;
    if (AS_OBJ(a)->type != AS_OBJ(b)->type) return false;
    
    switch (AS_OBJ(a)->type) {
        case OBJ_STRING: return AS_OBJ_TYPE(a, ObjString)->stringValue == 
                                AS_OBJ_TYPE(b, ObjString)->stringValue;
        default:         return AS_OBJ(a) == AS_OBJ(b);
    }
}

void printValue(Value value) {
    if (IS_NUMBER(value)) {
        double n = AS_NUMBER(value);
        if (n == (long long)n) printf("%lld", (long long)n);
        else printf("%.14g", n);
    }
    else if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    }
    else if (IS_NULL(value)) {
        printf("null");
    }
    else switch(AS_OBJ(value)->type) {
        case OBJ_STRING: printf("%s", AS_CSTRING(value)); break;
        case OBJ_RANGE: {
            ObjRange* r = AS_OBJ_TYPE(value, ObjRange);
            printf("<range:%g->%g   step:%g>", r->left, r->right, r->step);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* fn = AS_OBJ_TYPE(value, ObjFunction);
            if (fn->name == nullptr) printf("<script>");
            else printf("<func %s>", fn->name->stringValue.c_str());
            break;
        }
        default: printf("<unknown obj>"); break;
    }
}