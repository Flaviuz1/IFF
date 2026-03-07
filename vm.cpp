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

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instrucion = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instrucion];
    fprintf(stderr, "[line %d] in script\n", line);

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

void freeObjects() {
    Obj* obj = vm.objects;
    while (obj != nullptr) {
        Obj* next = obj->next;
        delete obj;
        obj = next;
    }
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void initVM(){
    resetStack();
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

static InterpretResult run() { // to be made faster after finishing
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define READ_CONSTANT_BIG() \
        (vm.chunk->constants.values[ \
            ((uint32_t)READ_BYTE() << 16) | \
            ((uint32_t)READ_BYTE() << 8)  | \
            ((uint32_t)READ_BYTE())          \
        ])
    #define NEGATE(valueType) (*(vm.stackTop - 1) = valueType(-AS_NUMBER(*(vm.stackTop - 1))))
    #define BANG(valueType) (*(vm.stackTop - 1) = valueType(isFalsey(*(vm.stackTop - 1))))
    #define CREMENT(valueType, delta) do{ \
        if(!IS_NUMBER(peek(0))) { \
           runtimeError("Operand must be a number."); \
           return INTERPRET_RUNTIME_ERROR; \
        } \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) + delta); \
    } while(false)
    
    #define BINARY_OP(valueType, op) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1)) ) { \
           runtimeError("Operands must be numbers (or strings for +/*)"); \
           return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) op b); \
    } while(false)

    #define EQUAL_CHECK(valueType, negate) do{ \
        Value b = pop(); \
        *(vm.stackTop - 1) = valueType(negate valuesEqual(*(vm.stackTop - 1), b)); \
    } while(false)

    #define MOD_POWER(valueType, op) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
           runtimeError("Operands must be numbers."); \
           return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(op(AS_NUMBER(*(vm.stackTop - 1)), b)); \
    } while(false)

    #define BITWISE_OP(valueType, op) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        int b = (int)AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType((double)((int)AS_NUMBER(*(vm.stackTop - 1)) op b)); \
    } while(false)

    #define STRING_ADD() do { \
        Value _b = pop(); \
        std::string result = valueToString(*(vm.stackTop - 1)) + valueToString(_b); \
        *(vm.stackTop - 1) = STRING_VAL(allocateString(result)); \
    } while(false)

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
    } while(false)

    for(;;){
        #ifdef DEBUG_TRACE_EXECUTION
            printf("             ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(*slot);
                printf(" ]");
            }
            printf("\n");
            disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()){
            //Constants
            case OP_CONSTANT:     {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_BIG: {
                push(READ_CONSTANT_BIG());
                break;
            }
            //Binary operators
            case OP_ADD:          {
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) { STRING_ADD(); }
                else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) { BINARY_OP(NUMBER_VAL, +); }
                else { runtimeError("Operands must be two numbers or at least one string."); return INTERPRET_RUNTIME_ERROR; }
                break;
            }
            case OP_SUBTRACT:     {BINARY_OP(NUMBER_VAL, -);         break;}
            case OP_MULTIPLY:     {
                if (IS_STRING(peek(0)) && IS_NUMBER(peek(1))) { STRING_MULTIPLY(); }
                else if (IS_NUMBER(peek(0)) && IS_STRING(peek(1))) { STRING_MULTIPLY(); }
                else { BINARY_OP(NUMBER_VAL, *); }
                break;
            }
            case OP_DIVIDE:       {BINARY_OP(NUMBER_VAL, /);         break;}
            case OP_POWER:        {MOD_POWER(NUMBER_VAL, pow);       break;}
            case OP_MODULO:       {MOD_POWER(NUMBER_VAL, fmod);      break;}
            case OP_INCREMENT:    {CREMENT(NUMBER_VAL, 1);           break;}
            case OP_DECREMENT:    {CREMENT(NUMBER_VAL, -1);          break;}
            case OP_SHIFT_LEFT:   {BITWISE_OP(NUMBER_VAL, <<);       break;}
            case OP_SHIFT_RIGHT:  {BITWISE_OP(NUMBER_VAL, >>);       break;}
            case OP_BITWISE_AND:  {BITWISE_OP(NUMBER_VAL, &);        break;}
            case OP_BITWISE_OR:   {BITWISE_OP(NUMBER_VAL, |);        break;}
            case OP_BITWISE_XOR:  {BITWISE_OP(NUMBER_VAL, ^);        break;}
            case OP_BITWISE_NOT:  {
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                *(vm.stackTop - 1) = NUMBER_VAL((double)(~(int)AS_NUMBER(*(vm.stackTop - 1))));
                break;
            }
            //Boolean stuff
            case OP_NEGATE:       {
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                NEGATE(NUMBER_VAL);
                break;
            }
            case OP_TRUE:         {
                push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE:        {
                push(BOOL_VAL(false));
                break;
            }
            case OP_NULL:         {
                push(NULL_VAL);
                break;
            }
            case OP_NOT:          {BANG(BOOL_VAL);                   break;}
            case OP_OR:           {break;}
            case OP_AND:          {break;}
            //Comparison
            case OP_GREATER:      {BINARY_OP(BOOL_VAL, >);           break;}
            case OP_GREATER_EQUAL:{BINARY_OP(BOOL_VAL, >=);          break;}
            case OP_LESS:         {BINARY_OP(BOOL_VAL, <);           break;}
            case OP_LESS_EQUAL:   {BINARY_OP(BOOL_VAL, <=);          break;}
            case OP_IS: {
                Value right = pop();
                *(vm.stackTop-1) = BOOL_VAL(sameType(*(vm.stackTop-1), right));
                break;
            }
            //Equality
            case OP_EQUAL_EQUAL:  {EQUAL_CHECK(BOOL_VAL,  );         break;}
            case OP_BANG_EQUAL:   {EQUAL_CHECK(BOOL_VAL, !);         break;}
            //Variables
            case OP_DEFINE_GLOBAL:{
                std::string name = AS_STRING(READ_CONSTANT())->stringValue;
                bool isConst = (READ_BYTE() == OP_CONST);
                vm.globals[name] = {pop(), isConst};
                break;
            }
            case OP_GET_GLOBAL:   {
                std::string name = AS_STRING(READ_CONSTANT())->stringValue;
                auto it = vm.globals.find(name);
                if (it == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(it->second.value);
                break;
            }
            case OP_SET_GLOBAL:   {
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
            case OP_DEFINE_GLOBAL_BIG:{
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
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL_BIG: {
                uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                        slot |= (uint32_t)READ_BYTE() << 8;
                        slot |= (uint32_t)READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL_BIG: {
                uint32_t slot  = (uint32_t)READ_BYTE() << 16;
                        slot |= (uint32_t)READ_BYTE() << 8;
                        slot |= (uint32_t)READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            //Misc
            case OP_STRINGIFY:    {
                if (!IS_STRING(peek(0))) {
                    *(vm.stackTop - 1) = STRING_VAL(allocateString(valueToString(*(vm.stackTop - 1))));
                }
                break;
            }
            case OP_PRINT_PLACEHOLDER: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_POP:          {pop(); break;}
            //Return
            case OP_RETURN:       {
                //exit interpreter
                return INTERPRET_OK;
            }
            default:
                runtimeError("Unknown opcode %d.", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_CONSTANT_BIG
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
    Chunk chunk;
    initChunk(&chunk);

    if(!compile(source, &chunk)){
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}