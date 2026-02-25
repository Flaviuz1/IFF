#include "common.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "value.hpp"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 3.14, 1);
    writeChunk(&chunk, OP_RETURN, 1);

    for (int i = 0; i < 300; i++)
    {
        writeConstant(&chunk, (double)i, 2);
    }

    writeChunk(&chunk, OP_RETURN, 2);

    disassembleChunk(&chunk, "test chunk");

    freeChunk(&chunk);
    return 0;
}
