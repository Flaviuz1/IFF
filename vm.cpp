#include <cstdio>
#include <cmath>
#include "vm.hpp"
#include "debug.hpp"
#include "common.hpp"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void push(Value value){
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop(){
    vm.stackTop--;
    return *vm.stackTop;
}

void initVM(){
    resetStack();
}

void freeVM(){

}

static InterpretResult run() { // to be made faster after finishing
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define BINARY_OP(op) do{ double b = pop(); *(vm.stackTop - 1) = *(vm.stackTop - 1) op b; } while(false)
    #define POWER_RAISE() do{ double b = pop(); *(vm.stackTop - 1) = pow(*(vm.stackTop - 1), b); } while(false)
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
            case OP_CONSTANT: {
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
            case OP_NEGATE: {
                *(vm.stackTop - 1) *= -1;
                break;
            }
            case OP_ADD:          {BINARY_OP(+);  break;}
            case OP_SUBTRACT:     {BINARY_OP(-);  break;}
            case OP_MULTIPLY:     {BINARY_OP(*);  break;}
            case OP_DIVIDE:       {BINARY_OP(/);  break;}
            case OP_RAISETOPOWER: {POWER_RAISE(); break;}
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef BINARY_OP
    #undef POWER_RAISE
}

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}