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
    str->value = value;
    str->next = vm.objects;
    vm.objects = str;
    return str;
}

