#define _USE_MATH_DEFINES
#include <cstdio>
#include <cmath>
#include <string>
#include <cstdarg>
#include "vm.hpp"
#include "value.hpp"
#include "debug.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "natives.hpp"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
         fprintf(stderr, "%s()\n", function->name->stringValue.c_str());
        }
    }

    resetStack();
}

void push(Value value){
    if (vm.stackTop >= vm.stack + STACK_MAX) {
        printf("Stack overflow!\n");
        exit(1);
    }
    *vm.stackTop++ = value;
}

Value pop(){
    vm.stackTop--;
    return *vm.stackTop;
}

static void freeObject(Obj* obj) {
    switch (obj->type) {
        case OBJ_FUNCTION:
            freeChunk(&((ObjFunction*)obj)->chunk);
            delete (ObjFunction*)obj;
            break;
        case OBJ_STRING: delete (ObjString*)obj; break;
        case OBJ_RANGE:  delete (ObjRange*)obj;  break;
        case OBJ_NATIVE: delete (ObjNative*)obj; break;
    }
}

void freeObjects() {
    Obj* obj = vm.objects;
    while (obj != nullptr) {
        Obj* next = obj->next;
        freeObject(obj);
        obj = next;
    }
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void defineNative(const char* name, NativeFn function, int arity = -1) {
    ObjNative* native = newNative(function, arity, name);
    vm.globals[std::string(name)] = {OBJ_VAL(native), false};
}

void initVM(){
    resetStack();
    //constants
    vm.globals["MATH_PI"]  = varAtt{NUMBER_VAL(M_PI),    true};
    vm.globals["MATH_E"]   = varAtt{NUMBER_VAL(M_E),     true};
    vm.globals["MATH_INF"] = varAtt{NUMBER_VAL(INFINITY), true};
    vm.globals["MATH_NAN"] = varAtt{NUMBER_VAL(NAN),      true};
    //io
    defineNative("clock", clockNative, 0);
    defineNative("print", printNative, 1);
    defineNative("printn",printnNative,1);
    defineNative("input", inputNative,-1);
    //math
    defineNative("abs",   absNative,   1);
    defineNative("max",   maxNative,  -1);
    defineNative("min",   minNative,  -1);
    defineNative("floor", floorNative, 1);
    defineNative("ceil",  ceilNative,  1);
    defineNative("round", roundNative, 1);
    defineNative("trunc", truncNative, 1);
    defineNative("sign",  signNative,  1);
    defineNative("fract", fractNative, 1);
    defineNative("clamp", clampNative, 3);
    defineNative("lerp",  lerpNative,  3);
    defineNative("exp",   expNative,   1);
    defineNative("ln",    logNative,   1);
    defineNative("log2",  log2Native,  1);
    defineNative("log10", log10Native, 1);
    defineNative("sin",   sinNative,   1);
    defineNative("cos",   cosNative,   1);
    defineNative("tan",   tanNative,   1);
    defineNative("asin",  asinNative,  1);
    defineNative("acos",  acosNative,  1);
    defineNative("atan",  atanNative,  1);
    defineNative("atan2", atan2Native, 2);
    //random

    //arrays

    //strings

    vm.objects = nullptr;
}

void freeVM(){
    freeObjects();
}

static inline std::string formatNumber(double n) {
    if (n == (int)n) return std::to_string((int)n);
    std::string s = std::to_string(n);
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    return s;
}

inline std::string valueToString(Value v) {
    switch (v.type) {
        case VAL_NUMBER: return formatNumber(AS_NUMBER(v));
        case VAL_BOOL:   return AS_BOOL(v) ? "true" : "false";
        case VAL_NULL:   return "null";
        case VAL_OBJ:
            switch (OBJ_TYPE(v)) {
                case OBJ_STRING: return AS_STRING(v)->stringValue;
                default:         return "<obj>";
            }
        default: return "";
    }
}

inline bool sameType(Value a, Value b){
    if (IS_OBJ(a) && IS_OBJ(b)) return (OBJ_TYPE(a) == OBJ_TYPE(b));
    return a.type == b.type;
}

static bool call(ObjFunction* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }
    if (vm.frameCount == FRAME_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;

    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                Value result = native->function(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    //frame->ip, frame->function->chunk FROM vm.ip, vm.chunk
    #define READ_BYTE()         (*frame->ip++)
    #define READ_CONSTANT()     (frame->function->chunk.constants.values[READ_BYTE()])
    #define READ_CONSTANT_BIG() \
        (frame->function->chunk.constants.values[ \
            ((uint32_t)READ_BYTE() << 16) | \
            ((uint32_t)READ_BYTE() << 8)  | \
            ((uint32_t)READ_BYTE())          \
        ])
    //frame->ip FROM vm.ip
    #define READ_24BITS() \
        (frame->ip += 3, \
         ((uint32_t)(frame->ip[-3]) << 16) | \
         ((uint32_t)(frame->ip[-2]) << 8)  | \
          (uint32_t)(frame->ip[-1]))

    #define NEGATE(valueType)   (*(vm.stackTop - 1) = valueType(-AS_NUMBER(*(vm.stackTop - 1))))
    #define BANG(valueType)     (*(vm.stackTop - 1) = valueType(isFalsey(*(vm.stackTop - 1))))

    #define CREMENT(valueType, delta) do { \
        if (!IS_NUMBER(peek(0))) { \
            runtimeError("Operand must be a number."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) + delta); \
    } while (false)

    #define BINARY_OP(valueType, op) do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers (or strings for +/*)"); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) op b); \
    } while (false)

    #define EQUAL_CHECK(valueType, negate) do { \
        Value b = pop(); \
        *(vm.stackTop - 1) = valueType(negate valuesEqual(*(vm.stackTop - 1), b)); \
    } while (false)

    #define MOD_POWER(valueType, op) do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(op(AS_NUMBER(*(vm.stackTop - 1)), b)); \
    } while (false)

    #define BITWISE_OP(valueType, op) do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        int b = (int)AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType((double)((int)AS_NUMBER(*(vm.stackTop - 1)) op b)); \
    } while (false)

    #define STRING_ADD() do { \
        Value _b = pop(); \
        std::string result = valueToString(*(vm.stackTop - 1)) + valueToString(_b); \
        *(vm.stackTop - 1) = STRING_VAL(allocateString(result)); \
    } while (false)

    #define STRING_MULTIPLY() do { \
        if (IS_NUMBER(peek(0))) { \
            int n = std::max(0, (int)AS_NUMBER(pop())); \
            std::string result; \
            result.reserve(AS_STRING(*(vm.stackTop-1))->stringValue.size() * n); \
            for (int i = 0; i < n; i++) result += AS_STRING(*(vm.stackTop-1))->stringValue; \
            *(vm.stackTop - 1) = STRING_VAL(allocateString(result)); \
        } else { \
            std::string s = AS_STRING(pop())->stringValue; \
            int n = std::max(0, (int)AS_NUMBER(*(vm.stackTop - 1))); \
            std::string result; \
            result.reserve(s.size() * n); \
            for (int i = 0; i < n; i++) result += s; \
            *(vm.stackTop - 1) = STRING_VAL(allocateString(result)); \
        } \
    } while (false)

    for (;;) {
        #ifdef DEBUG_TRACE_EXECUTION
            printf("             ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            disassembleInstruction(&frame->function->chunk,
                (int)(frame->ip - frame->function->chunk.code));
        #endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {

            case OP_CONSTANT:     { push(READ_CONSTANT());            break; }
            case OP_CONSTANT_BIG: { push(READ_CONSTANT_BIG());        break; }

            case OP_ADD: {
                if      (IS_STRING(peek(0)) || IS_STRING(peek(1)))            { STRING_ADD(); }
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))            { BINARY_OP(NUMBER_VAL, +); }
                else { runtimeError("Operands must be two numbers or at least one string."); return INTERPRET_RUNTIME_ERROR; }
                break;
            }
            case OP_SUBTRACT:     { BINARY_OP(NUMBER_VAL, -);          break; }
            case OP_MULTIPLY: {
                if      (IS_STRING(peek(0)) && IS_NUMBER(peek(1)))            { STRING_MULTIPLY(); }
                else if (IS_NUMBER(peek(0)) && IS_STRING(peek(1)))            { STRING_MULTIPLY(); }
                else                                                           { BINARY_OP(NUMBER_VAL, *); }
                break;
            }
            case OP_DIVIDE:       { BINARY_OP(NUMBER_VAL, /);          break; }
            case OP_POWER:        { MOD_POWER(NUMBER_VAL, pow);        break; }
            case OP_MODULO:       { MOD_POWER(NUMBER_VAL, fmod);       break; }
            case OP_INCREMENT:    { CREMENT(NUMBER_VAL,  1);           break; }
            case OP_DECREMENT:    { CREMENT(NUMBER_VAL, -1);           break; }
            case OP_SHIFT_LEFT:   { BITWISE_OP(NUMBER_VAL, <<);        break; }
            case OP_SHIFT_RIGHT:  { BITWISE_OP(NUMBER_VAL, >>);        break; }
            case OP_BITWISE_AND:  { BITWISE_OP(NUMBER_VAL, &);         break; }
            case OP_BITWISE_OR:   { BITWISE_OP(NUMBER_VAL, |);         break; }
            case OP_BITWISE_XOR:  { BITWISE_OP(NUMBER_VAL, ^);         break; }
            case OP_BITWISE_NOT: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                *(vm.stackTop - 1) = NUMBER_VAL((double)(~(int)AS_NUMBER(*(vm.stackTop - 1))));
                break;
            }
            //unary / literals
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                NEGATE(NUMBER_VAL);
                break;
            }
            case OP_NOT:          { BANG(BOOL_VAL);  break; }
            case OP_TRUE:         { push(BOOL_VAL(true));  break; }
            case OP_FALSE:        { push(BOOL_VAL(false)); break; }
            case OP_NULL:         { push(NULL_VAL);        break; }
            //comparison
            case OP_GREATER:       { BINARY_OP(BOOL_VAL, >);  break; }
            case OP_GREATER_EQUAL: { BINARY_OP(BOOL_VAL, >=); break; }
            case OP_LESS:          { BINARY_OP(BOOL_VAL, <);  break; }
            case OP_LESS_EQUAL:    { BINARY_OP(BOOL_VAL, <=); break; }
            case OP_EQUAL_EQUAL:   { EQUAL_CHECK(BOOL_VAL,  ); break; }
            case OP_BANG_EQUAL:    { EQUAL_CHECK(BOOL_VAL, !); break; }
            case OP_IS: {
                Value right = pop();
                *(vm.stackTop - 1) = BOOL_VAL(sameType(*(vm.stackTop - 1), right));
                break;
            }
            case OP_OR:  { break; }
            case OP_AND: { break; }
            //globals
            case OP_DEFINE_GLOBAL: {
                std::string name = AS_STRING(READ_CONSTANT())->stringValue;
                bool isConst = (READ_BYTE() == OP_CONST);
                vm.globals[name] = {pop(), isConst};
                break;
            }
            case OP_GET_GLOBAL: {
                std::string name = AS_STRING(READ_CONSTANT())->stringValue;
                auto it = vm.globals.find(name);
                if (it == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(it->second.value);
                break;
            }
            case OP_SET_GLOBAL: {
                std::string name = AS_STRING(READ_CONSTANT())->stringValue;
                auto it = vm.globals.find(name);
                if (it == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (it->second.isConst) {
                    runtimeError("Cannot reassign value to a constant.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                it->second.value = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL_BIG: {
                std::string name = AS_STRING(READ_CONSTANT_BIG())->stringValue;
                bool isConst = (READ_BYTE() == OP_CONST);
                vm.globals[name] = {pop(), isConst};
                break;
            }
            case OP_GET_GLOBAL_BIG: {
                std::string name = AS_STRING(READ_CONSTANT_BIG())->stringValue;
                auto it = vm.globals.find(name);
                if (it == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(it->second.value);
                break;
            }
            case OP_SET_GLOBAL_BIG: {
                std::string name = AS_STRING(READ_CONSTANT_BIG())->stringValue;
                auto it = vm.globals.find(name);
                if (it == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (it->second.isConst) {
                    runtimeError("Cannot reassign value to a constant.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                it->second.value = peek(0);
                break;
            }
            //locals
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL_BIG: {
                uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                         slot |= (uint32_t)READ_BYTE() << 8;
                         slot |= (uint32_t)READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL_BIG: {
                uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                         slot |= (uint32_t)READ_BYTE() << 8;
                         slot |= (uint32_t)READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            //ifs
            case OP_JUMP: {
                uint32_t offset = READ_24BITS();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint32_t offset = READ_24BITS();
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint32_t offset = READ_24BITS();
                frame->ip -= offset;
                break;
            }
            //fors
            case OP_RANGE: {
                double step  = AS_NUMBER(pop());
                double right = AS_NUMBER(pop());
                double left  = AS_NUMBER(pop());
                push(RANGE_VAL(allocateRange(left, right, step)));
                break;
            }
            case OP_FOR_ITERATE: {
                ObjRange* range = AS_RANGE(peek(1));
                double direction = range->right - range->left;
                if (direction == 0 ||
                    (direction < 0 && range->current <= range->right) ||
                    (direction > 0 && range->current >= range->right)) {
                    uint32_t offset = READ_24BITS();
                    frame->ip += offset;
                    break;
                }
                frame->ip += 3;
                *(vm.stackTop - 1) = NUMBER_VAL(range->current);
                range->current = range->left + (range->iterations * range->step);
                range->iterations++;
                break;
            }
            //functions
            case OP_CALL: {
                int argCount = READ_BYTE();
                if(!callValue(peek(argCount), argCount)) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            //misc
            case OP_STRINGIFY: {
                if (!IS_STRING(peek(0))) {
                    *(vm.stackTop - 1) = STRING_VAL(allocateString(valueToString(*(vm.stackTop - 1))));
                }
                break;
            }
            case OP_POP: { pop(); break; }
            case OP_DUP: { push(peek(0)); break; }

            //return
            case OP_RETURN: {
                Value result = pop();

                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stackTop = frame->slots;
                push(result);

                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            default:
                runtimeError("Unknown opcode %d.", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_CONSTANT_BIG
    #undef READ_24BITS
    #undef BINARY_OP
    #undef NEGATE
    #undef BANG
    #undef CREMENT
    #undef EQUAL_CHECK
    #undef MOD_POWER
    #undef STRING_ADD
    #undef STRING_MULTIPLY
    #undef BITWISE_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));

    callValue(OBJ_VAL(function), 0);

    return run();
}