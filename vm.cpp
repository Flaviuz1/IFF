#include <cstdio>
#include <cmath>
#include <string>
#include <cstdarg>
#include "vm.hpp"
#include "debug.hpp"
#include "common.hpp"
#include "compiler.hpp"

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

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
    return IS_NULL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL: return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        default:
            return false;
    }
}

void initVM(){
    resetStack();
}

void freeVM(){

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
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
           runtimeError("Operands must be numbers."); \
           return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) op b); \
    } while(false)
    
    #define BIT_SHIFT_NORMAL(valueType, way) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
           runtimeError("Operands must be numbers."); \
           return INTERPRET_RUNTIME_ERROR; \
        } \
        int b = (int)AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType((double)((int)AS_NUMBER(*(vm.stackTop - 1)) way b)); \
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
            case OP_ADD:          {BINARY_OP(NUMBER_VAL, +);         break;}
            case OP_SUBTRACT:     {BINARY_OP(NUMBER_VAL, -);         break;}
            case OP_MULTIPLY:     {BINARY_OP(NUMBER_VAL, *);         break;}
            case OP_DIVIDE:       {BINARY_OP(NUMBER_VAL, /);         break;}
            case OP_POWER:        {MOD_POWER(NUMBER_VAL, pow);       break;}
            case OP_MODULO:       {MOD_POWER(NUMBER_VAL, fmod);      break;}
            case OP_INCREMENT:    {CREMENT(NUMBER_VAL, 1);           break;}
            case OP_DECREMENT:    {CREMENT(NUMBER_VAL, -1);          break;}
            case OP_SHIFT_LEFT:   {BIT_SHIFT_NORMAL(NUMBER_VAL, <<); break;}
            case OP_SHIFT_RIGHT:  {BIT_SHIFT_NORMAL(NUMBER_VAL, >>); break;}
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
    #undef BIT_SHIFT_NORMAL
    #undef EQUAL_CHECK
    #undef MOD
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