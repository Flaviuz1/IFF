#define _USE_MATH_DEFINES
#include <cstdio>
#include <cmath>
#include <string>
#include <cstdarg>
#include <ctime>
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
    int idx = resolveGlobal(std::string(name));
    if (idx >= (int)vm.globals.size()) vm.globals.resize(idx + 1);
    vm.globals[idx] = {OBJ_VAL(native), false};
}

void initVM(){
    resetStack();
    vm.objects = nullptr;
    srand((unsigned int)time(nullptr));
    auto defineGlobal = [&](const char* name, Value value, bool isConst) {
        int idx = resolveGlobal(std::string(name));
        if (idx >= (int)vm.globals.size()) vm.globals.resize(idx + 1);
        vm.globals[idx] = {value, isConst};
    };
    //constants
    defineGlobal("MATH_PI",  NUMBER_VAL(M_PI),    true);
    defineGlobal("MATH_E",   NUMBER_VAL(M_E),     true);
    defineGlobal("MATH_INF", NUMBER_VAL(INFINITY), true);
    defineGlobal("MATH_NAN", NUMBER_VAL(NAN),      true);
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
    defineNative("random",  randomNative,   0);
    defineNative("seedset", seedNative,     1);
    defineNative("randomin",randominNative, 2);
    //arrays

    //strings
    defineNative("len",         lenNative,         1);
    defineNative("upper",       upperNative,        1);
    defineNative("lower",       lowerNative,        1);
    defineNative("trim",        trimNative,         1);
    defineNative("substr",      substrNative,       3);
    defineNative("contains",    containsNative,     2);
    defineNative("indexOf",     indexOfNative,      2);
    defineNative("charAt",      charAtNative,       2);
    defineNative("charCode",    charCodeNative,     2);
    defineNative("fromCharCode",fromCharCodeNative, 1);
    defineNative("replace",     replaceNative,      3);
    defineNative("repeat",      repeatNative,       2);
    defineNative("startsWith",  startsWithNative,   2);
    defineNative("endsWith",    endsWithNative,     2);

    //type conversions
    defineNative("toNumber", toNumberNative, 1);
    defineNative("toString", toStringNative, 1);
    defineNative("toBool",   toBoolNative,   1);
    defineNative("typeOf",   typeOfNative,   1);
    defineNative("isNumber", isNumberNative, 1);
    defineNative("isString", isStringNative, 1);
    defineNative("isBool",   isBoolNative,   1);
    defineNative("isNull",   isNullNative,   1);
    defineNative("isFunc",   isFuncNative,   1);
    defineNative("isNaN",    isNaNNative,    1);
    defineNative("isInf",    isInfNative,    1);
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
    if (IS_NUMBER(v)) return formatNumber(AS_NUMBER(v));
    else if (IS_BOOL(v)) return AS_BOOL(v) ? "true" : "false";
    else if (IS_NULL(v)) return "null";
    else if (IS_OBJ(v)) return AS_OBJ_TYPE(v, ObjString)->stringValue;
    else return "unknown";
}

inline bool sameType(Value a, Value b){
    if (IS_NUMBER(a) && IS_NUMBER(b)) return true;
    else if (IS_BOOL(a) && IS_BOOL(b)) return true;
    else if (IS_NULL(a) && IS_NULL(b)) return true;
    else if (IS_OBJ(a) && IS_OBJ(b)) return AS_OBJ(a)->type == AS_OBJ(b)->type;
    else return false;
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
        switch (AS_OBJ(callee)->type) {
            case OBJ_FUNCTION:
                return call(AS_OBJ_TYPE(callee, ObjFunction), argCount);
            case OBJ_NATIVE: {
                ObjNative* native = AS_OBJ_TYPE(callee, ObjNative);
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
    Value* stackTop = vm.stackTop;

    #define PUSH(v)   (*stackTop++ = (v))
    #define POP()     (*(--stackTop))
    #define PEEK(d)   (stackTop[-1-(d)])
    #define SYNC()    (vm.stackTop = stackTop)
    #define RELOAD()  (stackTop = vm.stackTop)

    static void* table[] = {
        &&L_OP_RETURN,           // 0
        &&L_OP_CONSTANT,         // 1
        &&L_OP_CONSTANT_BIG,     // 2
        &&L_OP_NULL,             // 3
        &&L_OP_TRUE,             // 4
        &&L_OP_FALSE,            // 5
        &&L_OP_NEGATE,           // 6
        &&L_OP_ADD,              // 7
        &&L_OP_SUBTRACT,         // 8
        &&L_OP_MULTIPLY,         // 9
        &&L_OP_DIVIDE,           // 10
        &&L_OP_MODULO,           // 11
        &&L_OP_POWER,            // 12
        &&L_OP_INCREMENT,        // 13
        &&L_OP_DECREMENT,        // 14
        &&L_OP_SHIFT_LEFT,       // 15
        &&L_OP_SHIFT_RIGHT,      // 16
        &&L_OP_BITWISE_AND,      // 17
        &&L_OP_BITWISE_OR,       // 18
        &&L_OP_BITWISE_XOR,      // 19
        &&L_OP_BITWISE_NOT,      // 20
        &&L_OP_EQUAL_EQUAL,      // 21
        &&L_OP_BANG_EQUAL,       // 22
        &&L_OP_GREATER,          // 23
        &&L_OP_GREATER_EQUAL,    // 24
        &&L_OP_LESS,             // 25
        &&L_OP_LESS_EQUAL,       // 26
        &&L_OP_IS,               // 27
        &&L_OP_NOT,              // 28
        &&L_OP_AND,              // 29
        &&L_OP_OR,               // 30
        &&L_OP_DEFINE_GLOBAL,    // 31
        &&L_OP_SET_GLOBAL,       // 32
        &&L_OP_GET_GLOBAL,       // 33
        &&L_OP_DEFINE_GLOBAL_BIG,// 34
        &&L_OP_SET_GLOBAL_BIG,   // 35
        &&L_OP_GET_GLOBAL_BIG,   // 36
        &&L_OP_GET_LOCAL,        // 37
        &&L_OP_GET_LOCAL_BIG,    // 38
        &&L_OP_SET_LOCAL,        // 39
        &&L_OP_SET_LOCAL_BIG,    // 40
        &&L_OP_CONST,            // 41
        &&L_OP_NOT_CONST,        // 42
        &&L_OP_JUMP_IF_FALSE,    // 43
        &&L_OP_JUMP,             // 44
        &&L_OP_LOOP,             // 45
        &&L_OP_RANGE,            // 46
        &&L_OP_BY,               // 47
        &&L_OP_FOR_ITERATE,      // 48
        &&L_OP_CALL,             // 49
        &&L_OP_POP,              // 50
        &&L_OP_DUP,              // 51
        &&L_OP_STRINGIFY,        // 52
    };

    // @section: Macros 
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

    #define NEGATE(valueType)   (*(stackTop - 1) = valueType(-AS_NUMBER(*(stackTop - 1))))
    #define BANG(valueType)     (*(stackTop - 1) = valueType(isFalsey(*(stackTop - 1))))

    #define CREMENT(valueType, delta) do { \
        Value _a = *(stackTop - 1); \
        if (!IS_NUMBER(_a)) { \
            runtimeError("Operand must be a number."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        *(stackTop - 1) = valueType(AS_NUMBER(_a) + delta); \
    } while (false)

    #define BINARY_OP(valueType, op) do { \
        Value _b = *(stackTop - 1); \
        Value _a = *(stackTop - 2); \
        if (!IS_NUMBER(_a) || !IS_NUMBER(_b)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        stackTop--; \
        *(stackTop - 1) = valueType(AS_NUMBER(_a) op AS_NUMBER(_b)); \
    } while (false)

    #define EQUAL_CHECK(valueType, negate) do { \
        Value _b = *(stackTop - 1); \
        stackTop--; \
        *(stackTop - 1) = valueType(negate valuesEqual(*(stackTop - 1), _b)); \
    } while (false)

    #define MOD_POWER(valueType, op) do { \
        Value _b = *(stackTop - 1); \
        Value _a = *(stackTop - 2); \
        if (!IS_NUMBER(_a) || !IS_NUMBER(_b)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        stackTop--; \
        *(stackTop - 1) = valueType(op(AS_NUMBER(_a), AS_NUMBER(_b))); \
    } while (false)

    #define BITWISE_OP(valueType, op) do { \
        Value _b = *(stackTop - 1); \
        Value _a = *(stackTop - 2); \
        if (!IS_NUMBER(_a) || !IS_NUMBER(_b)) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        stackTop--; \
        *(stackTop - 1) = valueType((double)((int)AS_NUMBER(_a) op (int)AS_NUMBER(_b))); \
    } while (false)

    #define STRING_ADD() do { \
        Value _b = *(stackTop - 1); \
        stackTop--; \
        std::string result = valueToString(*(stackTop - 1)) + valueToString(_b); \
        *(stackTop - 1) = OBJ_VAL(allocateString(result)); \
    } while (false)

    #define STRING_MULTIPLY() do { \
        Value _top = *(stackTop - 1); \
        Value _bot = *(stackTop - 2); \
        stackTop--; \
        std::string s   = IS_OBJ_TYPE(_bot, OBJ_STRING) ? AS_OBJ_TYPE(_bot, ObjString)->stringValue \
                                                        : AS_OBJ_TYPE(_top, ObjString)->stringValue; \
        int n = std::max(0, (int)AS_NUMBER(IS_NUMBER(_top) ? _top : _bot)); \
        std::string result; \
        result.reserve(s.size() * n); \
        for (int i = 0; i < n; i++) result += s; \
        *(stackTop - 1) = OBJ_VAL(allocateString(result)); \
    } while (false)

    #ifdef DEBUG_TRACE_EXECUTION
        #define DISPATCH() \
            do { \
                printf("          "); \
                for (Value* slot = vm.stack; slot < stackTop; slot++) { printf("[ "); printValue(*slot); printf(" ]"); } \
                printf("\n"); \
                disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code)); \
                goto *table[READ_BYTE()]; \
            } while(0)
    #else
        #define DISPATCH() goto *table[READ_BYTE()]
    #endif
    
    DISPATCH();
    // @endsection

    // @section: Ops
    L_OP_CONSTANT : {
        PUSH(READ_CONSTANT());
        DISPATCH();
    }
    L_OP_CONSTANT_BIG: {
        PUSH(READ_CONSTANT_BIG());
        DISPATCH();
    }
    L_OP_ADD: {
        Value b = PEEK(0);
        Value a = PEEK(1);
        if (IS_NUMBER(a) && IS_NUMBER(b)) {
            *(stackTop - 2) = NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b));
            stackTop--;
        } else if (IS_OBJ_TYPE(PEEK(0), OBJ_STRING) || IS_OBJ_TYPE(PEEK(1), OBJ_STRING)) {
            STRING_ADD();
        } else {
            runtimeError("Operands must be numbers or strings.");
            return INTERPRET_RUNTIME_ERROR;
        }
        DISPATCH();
    }
    L_OP_SUBTRACT: {
        BINARY_OP(NUMBER_VAL, -);
        DISPATCH();
    }
    L_OP_MULTIPLY: {
        if (IS_OBJ_TYPE(PEEK(0), OBJ_STRING) && IS_NUMBER(PEEK(1))) STRING_MULTIPLY();
        else if (IS_NUMBER(PEEK(0)) && IS_OBJ_TYPE(PEEK(1), OBJ_STRING)) STRING_MULTIPLY();
        else BINARY_OP(NUMBER_VAL, *);
        DISPATCH();
    }
    L_OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        DISPATCH();
    }
    L_OP_POWER:        { MOD_POWER(NUMBER_VAL, pow);  DISPATCH(); }
    L_OP_MODULO:       { MOD_POWER(NUMBER_VAL, fmod); DISPATCH(); }
    L_OP_INCREMENT:    { CREMENT(NUMBER_VAL,  1);     DISPATCH(); }
    L_OP_DECREMENT:    { CREMENT(NUMBER_VAL, -1);     DISPATCH(); }
    L_OP_SHIFT_LEFT:   { BITWISE_OP(NUMBER_VAL, <<);  DISPATCH(); }
    L_OP_SHIFT_RIGHT:  { BITWISE_OP(NUMBER_VAL, >>);  DISPATCH(); }
    L_OP_BITWISE_AND:  { BITWISE_OP(NUMBER_VAL, &);   DISPATCH(); }
    L_OP_BITWISE_OR:   { BITWISE_OP(NUMBER_VAL, |);   DISPATCH(); }
    L_OP_BITWISE_XOR:  { BITWISE_OP(NUMBER_VAL, ^);   DISPATCH(); }
    L_OP_BITWISE_NOT: {
        if (!IS_NUMBER(PEEK(0))) {
            runtimeError("Operand must be a number.");
            return INTERPRET_RUNTIME_ERROR;
        }
        *(stackTop - 1) = NUMBER_VAL((double)(~(int)AS_NUMBER(*(stackTop - 1))));
        DISPATCH();
    }
    L_OP_NEGATE: {
        if (!IS_NUMBER(PEEK(0))) {
            runtimeError("Operand must be a number.");
            return INTERPRET_RUNTIME_ERROR;
        }
        NEGATE(NUMBER_VAL);
        DISPATCH();
    }
    L_OP_NOT:          { BANG(BOOL_VAL);        DISPATCH(); }
    L_OP_TRUE:         { PUSH(TRUE_VAL);        DISPATCH(); }
    L_OP_FALSE:        { PUSH(FALSE_VAL);       DISPATCH(); }
    L_OP_NULL:         { PUSH(NULL_VAL);        DISPATCH(); }
    L_OP_GREATER:       { BINARY_OP(BOOL_VAL, >);  DISPATCH(); }
    L_OP_GREATER_EQUAL: { BINARY_OP(BOOL_VAL, >=); DISPATCH(); }
    L_OP_LESS:          { BINARY_OP(BOOL_VAL, <);  DISPATCH(); }
    L_OP_LESS_EQUAL:    { BINARY_OP(BOOL_VAL, <=); DISPATCH(); }
    L_OP_EQUAL_EQUAL:   { EQUAL_CHECK(BOOL_VAL,  ); DISPATCH(); }
    L_OP_BANG_EQUAL:    { EQUAL_CHECK(BOOL_VAL, !); DISPATCH(); }
    L_OP_IS: {
        Value right = POP();
        *(stackTop - 1) = BOOL_VAL(sameType(*(stackTop - 1), right));
        DISPATCH();
    }
    L_OP_OR:  { DISPATCH(); }
    L_OP_AND: { DISPATCH(); }
    L_OP_DEFINE_GLOBAL: {
        uint8_t idx = READ_BYTE();
        bool isConst = (READ_BYTE() == OP_CONST);
        if (idx >= vm.globals.size()) vm.globals.resize(idx + 1);
        vm.globals[idx] = {POP(), isConst};
        DISPATCH();
    }
    L_OP_GET_GLOBAL: {
        uint8_t idx = READ_BYTE();
        PUSH(vm.globals[idx].value);
        DISPATCH();
    }
    L_OP_SET_GLOBAL: {
        uint8_t idx = READ_BYTE();
        if (vm.globals[idx].isConst) {
            runtimeError("Cannot reassign value to a constant.");
            return INTERPRET_RUNTIME_ERROR;
        }
        vm.globals[idx].value = PEEK(0);
        DISPATCH();
    }
    L_OP_DEFINE_GLOBAL_BIG: {
        uint32_t idx = READ_24BITS();
        bool isConst = (READ_BYTE() == OP_CONST);
        if (idx >= vm.globals.size()) vm.globals.resize(idx + 1);
        vm.globals[idx] = {POP(), isConst};
        DISPATCH();
    }
    L_OP_GET_GLOBAL_BIG: {
        uint32_t idx = READ_24BITS();
        PUSH(vm.globals[idx].value);
        DISPATCH();
    }
    L_OP_SET_GLOBAL_BIG: {
        uint32_t idx = READ_24BITS();
        if (vm.globals[idx].isConst) {
            runtimeError("Cannot reassign value to a constant.");
            return INTERPRET_RUNTIME_ERROR;
        }
        vm.globals[idx].value = PEEK(0);
        DISPATCH();
    }
    L_OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        PUSH(frame->slots[slot]);
        DISPATCH();
    }
    L_OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = PEEK(0);
        DISPATCH();
    }
    L_OP_GET_LOCAL_BIG: {
        uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                 slot |= (uint32_t)READ_BYTE() << 8;
                 slot |= (uint32_t)READ_BYTE();
        PUSH(frame->slots[slot]);
        DISPATCH();
    }
    L_OP_SET_LOCAL_BIG: {
        uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                 slot |= (uint32_t)READ_BYTE() << 8;
                 slot |= (uint32_t)READ_BYTE();
        frame->slots[slot] = PEEK(0);
        DISPATCH();
    }
    L_OP_CONST:     { DISPATCH(); }
    L_OP_NOT_CONST: { DISPATCH(); }
    L_OP_JUMP: {
        uint32_t offset = READ_24BITS();
        frame->ip += offset;
        DISPATCH();
    }
    L_OP_JUMP_IF_FALSE: {
        uint32_t offset = READ_24BITS();
        if (isFalsey(PEEK(0))) frame->ip += offset;
        DISPATCH();
    }
    L_OP_LOOP: {
        uint32_t offset = READ_24BITS();
        frame->ip -= offset;
        DISPATCH();
    }
    L_OP_RANGE: {
        double step  = AS_NUMBER(POP());
        double right = AS_NUMBER(POP());
        double left  = AS_NUMBER(POP());
        PUSH(NUMBER_VAL(right));  // end
        PUSH(NUMBER_VAL(step));   // step
        PUSH(NUMBER_VAL(left));   // next
        PUSH(NUMBER_VAL(left));   // i
        DISPATCH();
    }
    L_OP_BY: { DISPATCH(); }
    L_OP_FOR_ITERATE: {
        double next = AS_NUMBER(*(stackTop - 2));
        double step = AS_NUMBER(*(stackTop - 3));
        double end  = AS_NUMBER(*(stackTop - 4));

        bool done = (step > 0) ? (next >= end)
                : (step < 0) ? (next <= end)
                : true;

        if (done) {
            uint32_t offset = READ_24BITS();
            frame->ip += offset;
            DISPATCH();
        }
        frame->ip += 3;
        *(stackTop - 1) = NUMBER_VAL(next);
        *(stackTop - 2) = NUMBER_VAL(next + step);
        DISPATCH();
    }
    L_OP_CALL: {
        int argCount = READ_BYTE();
        SYNC();
        if (!callValue(PEEK(argCount), argCount)) return INTERPRET_RUNTIME_ERROR;
        RELOAD();
        frame = &vm.frames[vm.frameCount - 1];
        DISPATCH();
    }
    L_OP_STRINGIFY: {
        if (!IS_OBJ_TYPE(PEEK(0), OBJ_STRING)) {
            *(stackTop - 1) = OBJ_VAL(allocateString(valueToString(*(stackTop - 1))));
        }
        DISPATCH();
    }
    L_OP_POP: { POP(); DISPATCH(); }
    L_OP_DUP: { PUSH(PEEK(0)); DISPATCH(); }
    L_OP_RETURN: {
        Value result = POP();
        vm.frameCount--;
        if (vm.frameCount == 0) {
            POP();
            return INTERPRET_OK;
        }
        stackTop = frame->slots;
        PUSH(result);
        frame = &vm.frames[vm.frameCount - 1];
        DISPATCH();
    }
    // @endsection
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