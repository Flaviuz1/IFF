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

static std::string formatNumber(double n) {
    if (n == (int)n) return std::to_string((int)n);
    std::string s = std::to_string(n);
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    return s;
}

std::string valueToString(Value v) {
    switch (v.type) {
        case VAL_NUMBER: return formatNumber(AS_NUMBER(v));
        case VAL_BOOL:   return AS_BOOL(v) ? "true" : "false";
        case VAL_NULL:   return "null";
        case VAL_OBJ:
            switch (OBJ_TYPE(v)) {
                case OBJ_STRING: return AS_STRING(v)->value;
                default:         return "<obj>";
            }
        default: return "";
    }
}

void initVM(){
    resetStack();
    vm.objects = nullptr;
}

void freeVM(){
    freeObjects();
}

static InterpretResult run() { // to be made faster after finishing
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
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
           runtimeError("Operands must be numbers or strings."); \
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
        std::string result = VALUE_TO_STRING(*(vm.stackTop - 1)) + VALUE_TO_STRING(_b); \
        *(vm.stackTop - 1) = STRING_VAL(allocateString(result)); \
    } while(false)

    #define STRING_MULTIPLY() do { \
        if (IS_NUMBER(peek(0))) { \
            int n = std::max(0, (int)AS_NUMBER(pop())); \
            ObjString* result = new ObjString(); \
            result->type = OBJ_STRING; \
            result->value.reserve(AS_STRING(*(vm.stackTop-1))->value.size() * n); \
            for (int i = 0; i < n; i++) result->value += AS_STRING(*(vm.stackTop-1))->value; \
            *(vm.stackTop - 1) = STRING_VAL(result); \
        } else { \
            std::string s = AS_STRING(pop())->value; \
            int n = std::max(0, (int)AS_NUMBER(*(vm.stackTop - 1))); \
            ObjString* result = new ObjString(); \
            result->type = OBJ_STRING; \
            result->value.reserve(s.size() * n); \
            for (int i = 0; i < n; i++) result->value += s; \
            *(vm.stackTop - 1) = STRING_VAL(result); \
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
                uint32_t idx  = (uint32_t)READ_BYTE() << 16;
                         idx |= (uint32_t)READ_BYTE() << 8;
                         idx |= (uint32_t)READ_BYTE();
                Value constant = vm.chunk->constants.values[idx];
                push(constant);
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
            //Assignment
            case OP_EQUAL:        {break;}
            //Comparison
            case OP_GREATER:      {BINARY_OP(BOOL_VAL, >);           break;}
            case OP_GREATER_EQUAL:{BINARY_OP(BOOL_VAL, >=);          break;}
            case OP_LESS:         {BINARY_OP(BOOL_VAL, <);           break;}
            case OP_LESS_EQUAL:   {BINARY_OP(BOOL_VAL, <=);          break;}
            //Equality
            case OP_EQUAL_EQUAL:  {EQUAL_CHECK(BOOL_VAL,  );         break;}
            case OP_BANG_EQUAL:   {EQUAL_CHECK(BOOL_VAL, !);         break;}
            //Misc
            case OP_STRINGIFY:    {
                if (!IS_STRING(peek(0))) {
                    *(vm.stackTop - 1) = STRING_VAL(allocateString(valueToString(*(vm.stackTop - 1))));
                }
                break;
            }
            //Return
            case OP_RETURN:       {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
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