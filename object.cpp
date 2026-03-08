#include <cstdio>
#include <cstring>
#include <string>
#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"
#include "vm.hpp"
#include "chunk.hpp"

extern VM vm;

ObjString* copyString(const char* chars, int length) {
    return allocateString(std::string(chars, length));
}

ObjString* allocateString(std::string value) {
    ObjString* str = new ObjString();
    str->type = OBJ_STRING;
    str->stringValue = value;
    str->next = vm.objects;
    vm.objects = str;
    return str;
}

ObjRange* allocateRange(double left, double right, double step) {
    ObjRange* range = new ObjRange();
    range->type = OBJ_RANGE;
    range->left = left;
    range->right = right;
    range->current = left;
    range->dir = (int)(right-left);
    range->step = step;
    range->next = vm.objects;
    vm.objects = range;
    return range;
}

ObjFunction* newFunction() {
    ObjFunction* fn = new ObjFunction();
    fn->type  = OBJ_FUNCTION;
    fn->arity = 0;
    fn->name  = nullptr;
    fn->next  = vm.objects;
    vm.objects = fn;
    initChunk(&fn->chunk);
    return fn;
}

static ObjNative* newNative(NativeFn function, int arity, const char* name) {
    ObjNative* native = new ObjNative();
    native->type     = OBJ_NATIVE;
    native->next     = vm.objects;
    native->function = function;
    native->arity    = arity;
    native->name     = name;
    vm.objects       = native;
    return native;
}