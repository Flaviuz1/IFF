#include <cstdio>
#include <cstring>
#include <string>
#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"
#include "vm.hpp"

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

Range* allocateRange(double left, double right, double step) {
    Range* range = new Range();
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
