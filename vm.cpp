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

void initVM(){
    resetStack();
}

void freeVM(){

}

static InterpretResult run() { // to be made faster after finishing
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    
    #define BINARY_OP(valueType, op) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
           runtimeError("Operands must be numbers."); \
           return INTERPRET_RUNTIME_ERROR; \ 
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(AS_NUMBER(*(vm.stackTop - 1)) op b); \
    } while(false)

    #define POWER_RAISE(valueType) do{ \
        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
           runtimeError("Operands must be numbers."); \
           return INTERPRET_RUNTIME_ERROR; \ 
        } \
        double b = AS_NUMBER(pop()); \
        *(vm.stackTop - 1) = valueType(pow(AS_NUMBER(*(vm.stackTop - 1)), b)); \
    } while(false)
    //no define for big constants because of irregularities in compiling

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
            case OP_ADD:          {BINARY_OP(NUMBER_VAL, +);  break;}
            case OP_SUBTRACT:     {BINARY_OP(NUMBER_VAL, -);  break;}
            case OP_MULTIPLY:     {BINARY_OP(NUMBER_VAL, *);  break;}
            case OP_DIVIDE:       {BINARY_OP(NUMBER_VAL, /);  break;}
            case OP_POWER:        {POWER_RAISE(NUMBER_VAL);   break;}
            case OP_NEGATE:       {
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_RETURN:       {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
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
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
    #undef POWER_RAISE
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