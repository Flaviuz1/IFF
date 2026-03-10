#pragma once

#include "common.hpp"
#include "value.hpp"
#include "vm.hpp"

ObjFunction* compile(const char* source);
int resolveGlobal(const std::string& name);