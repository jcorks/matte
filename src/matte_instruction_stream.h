/*
Copyright (c) 2026, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/

#ifndef H_MATTE__INSTRUCTION_STREAM__INCLUDED
#define H_MATTE__INSTRUCTION_STREAM__INCLUDED

#include <stdint.h>
#include "matte_bytecode_stub.h"

typedef struct matteArray_t matteArray_t;


enum {
    MATTE_INSTRUCTION_STREAM__OPTIONS__DEBUG = 1,
};

extern uint8_t MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[MATTE_OPCODE_COUNT];

void matte_instruction_stream_decode(
    matteBytecodeStubInstruction_t * instructions,
    uint32_t len,
    uint32_t * startLine,
    uint8_t *** bytesIn,
    uint32_t ** leftIn
);


// Converts a raw instruction stream into a compressed form 
// used for raw bytecode stubs. The returned buffer must be 
// freed with matte_deallocate(). The length in bytes is returned.
uint8_t * matte_instruction_stream_encode(
    matteArray_t * instructions,
    int includeDebug,
    uint32_t startingLine,
    uint32_t * len
);

#endif

