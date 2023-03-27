/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
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
#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_array.h"
#include "matte_opcode.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ASSUMES THAT STRING IDS AS GIVEN BY 
// THE HEAP PERSIST THROUGHOUT THE HEAP'S LIFETIME
// AS IN THE CURRENT IMPLEMENTATION. this will need 
// to change if the heap string handling changes!


struct matteBytecodeStub_t {
    uint32_t fileID;
    uint32_t stubID;
    
    matteValue_t * argNames;
    matteValue_t * localNames;
    matteValue_t * strings;

    matteBytecodeStubCapture_t * captures; 
    matteBytecodeStubInstruction_t * instructions;

    uint8_t argCount;
    uint8_t localCount;
    uint16_t capturedCount;
    uint32_t instructionCount;
    uint32_t stringCount;
};  

// prevents incomplete advances


#define ADVANCE(__T__, __V__) ADVANCE_SRC(sizeof(__T__), &(__V__), left, bytes);
#define ADVANCEN(__N__, __V__) ADVANCE_SRC(__N__, &(__V__), left, bytes);

static void ADVANCE_SRC(int n, void * ptr, uint32_t * left, uint8_t ** bytes) {
    int32_t v = n <= *left ? n : *left;
    memcpy(ptr, *bytes, v);
    *left-=v;
    *bytes+=v;
}


static matteValue_t chomp_string(matteHeap_t * heap, uint8_t ** bytes, uint32_t * left) {
    matteString_t * str = matte_string_create();
    uint32_t len = 0;
    ADVANCE(uint32_t, len);
    int32_t ch = 0;
    uint32_t i;
    for(i = 0; i < len; ++i) {
        ADVANCE(int32_t, ch);
        matte_string_append_char(str, ch);
    }
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_string(heap, &v, str);
    matte_string_destroy(str);
    return v;
}



static matteBytecodeStub_t * bytes_to_stub(matteHeap_t * heap, uint32_t fileID, uint8_t ** bytes, uint32_t * left) {
    matteBytecodeStub_t * out = calloc(1, sizeof(matteBytecodeStub_t));
    uint32_t i;
    uint8_t ver = 0;

    uint8_t tag[6];
    ADVANCEN(6*sizeof(uint8_t), tag);
    if (
        tag[0] != 'M'  ||
        tag[1] != 'A'  ||
        tag[2] != 'T'  ||
        tag[3] != 0x01 ||
        tag[4] != 0x06 ||
        tag[5] != 'B'
    ) {
        return out;
    }
    ADVANCE(uint8_t, ver);
    if (ver != 1) return out;


    out->fileID = fileID;
    ADVANCE(uint32_t, out->stubID);
    ADVANCE(uint8_t, out->argCount);
    out->argNames = malloc(sizeof(matteValue_t)*out->argCount);
    for(i = 0; i < out->argCount; ++i) {
        out->argNames[i] = chomp_string(heap, bytes, left);    
    }        
    ADVANCE(uint8_t, out->localCount);
    out->localNames = malloc(sizeof(matteValue_t)*out->localCount);
    for(i = 0; i < out->localCount; ++i) {
        out->localNames[i] = chomp_string(heap, bytes, left);    
    }        
    ADVANCE(uint32_t, out->stringCount);
    out->strings = malloc(sizeof(matteValue_t)*out->stringCount);
    for(i = 0; i < out->stringCount; ++i) {
        out->strings[i] = chomp_string(heap, bytes, left);    
    }       
    ADVANCE(uint16_t, out->capturedCount);
    if (out->capturedCount) {
        out->captures = calloc(sizeof(matteBytecodeStubCapture_t), out->capturedCount);
        ADVANCEN(sizeof(matteBytecodeStubCapture_t)*out->capturedCount, out->captures[0]);
    }
    ADVANCE(uint32_t, out->instructionCount);
    if (out->instructionCount) {
        out->instructions = calloc(sizeof(matteBytecodeStubInstruction_t), out->instructionCount);    
        for(i = 0; i < out->instructionCount; ++i) {
            ADVANCE(uint32_t, out->instructions[i].lineNumber);
            ADVANCE(uint8_t,  out->instructions[i].opcode);
            ADVANCEN(8, out->instructions[i].data[0]);
        }
    }

    // complete linkage for NFN instructions
    // This will help other instances know the originating
    for(i = 0; i < out->instructionCount; ++i) {
        if (out->instructions[i].opcode == MATTE_OPCODE_NFN) {
            *((uint32_t*)(&out->instructions[i].data[0])) = fileID;
        }
    }
    return out;    
}

void matte_bytecode_stub_destroy(matteBytecodeStub_t * b) {
    free(b->strings);   
    free(b->captures);
    free(b->instructions);
    free(b->localNames);
    free(b->argNames);
    free(b);
}


matteArray_t * matte_bytecode_stubs_from_bytecode(
    matteHeap_t * heap,
    uint32_t fileID,
    const uint8_t * bytecodeRaw, 
    uint32_t len
) {
    matteArray_t * arr = matte_array_create(sizeof(matteBytecodeStub_t *));
    while(len) {
        matteBytecodeStub_t * s = bytes_to_stub(heap, fileID, (uint8_t**)(&bytecodeRaw), &len);
        matte_array_push(arr, s);
    }
    return arr;
}



uint32_t matte_bytecode_stub_get_file_id(const matteBytecodeStub_t * stub) {
    return stub->fileID;
}

uint32_t matte_bytecode_stub_get_id(const matteBytecodeStub_t * stub) {
    return stub->stubID;
}


uint8_t matte_bytecode_stub_arg_count(const matteBytecodeStub_t * stub) {
    return stub->argCount;
}


uint8_t matte_bytecode_stub_local_count(const matteBytecodeStub_t * stub) {
    return stub->localCount;
}


matteValue_t matte_bytecode_stub_get_arg_name(const matteBytecodeStub_t * stub, uint8_t i) {    
    if (i >= stub->argCount) {
        matteValue_t v = {};
        return v;
    }
    return stub->argNames[i];
}

matteValue_t matte_bytecode_stub_get_local_name(const matteBytecodeStub_t * stub, uint8_t i) {
    if (i >= stub->localCount) {
        matteValue_t v = {};
        return v;
    }

    return stub->localNames[i];
}

matteValue_t matte_bytecode_stub_get_string(const matteBytecodeStub_t * stub, uint32_t i) {
    if (i >= stub->stringCount) {
        matteValue_t v = {};
        return v;
    }
    return stub->strings[i];
}


const matteBytecodeStubCapture_t * matte_bytecode_stub_get_captures(
    const matteBytecodeStub_t * stub, 
    uint32_t * count
) {
    *count = stub->capturedCount;
    return stub->captures;
}


const matteBytecodeStubInstruction_t * matte_bytecode_stub_get_instructions(const matteBytecodeStub_t * stub, uint32_t * count) {
    *count = stub->instructionCount;
    return stub->instructions;
}





