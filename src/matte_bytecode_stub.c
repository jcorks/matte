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
#include "matte.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ASSUMES THAT STRING IDS AS GIVEN BY 
// THE STORE PERSIST THROUGHOUT THE STORE'S LIFETIME
// AS IN THE CURRENT IMPLEMENTATION. this will need 
// to change if the store string handling changes!


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
    uint32_t startingLine;
    int isDynamicBinding;
    uint8_t isVarArg;

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


static matteValue_t chomp_string(matteStore_t * store, uint8_t ** bytes, uint32_t * left) {
    matteString_t * str;
    uint32_t len = 0;
    ADVANCE(uint32_t, len);
    uint8_t * utf8raw = (uint8_t*)matte_allocate((len+1)*1);
    ADVANCEN(len, utf8raw[0]);
    
    str = matte_string_create_from_c_str("%s", utf8raw);
    matte_deallocate(utf8raw);

    matteValue_t v = matte_store_new_value(store);
    matte_value_into_string(store, &v, str);
    matte_string_destroy(str);
    return v;
}

matteBytecodeStub_t * matte_bytecode_stub_create_symbolic() {
    matteBytecodeStub_t * out = (matteBytecodeStub_t*)matte_allocate(sizeof(matteBytecodeStub_t));
    out->isDynamicBinding = 1;
    return out;
}


static matteBytecodeStub_t * bytes_to_stub(matteStore_t * store, uint32_t fileID, uint8_t ** bytes, uint32_t * left) {
    matteBytecodeStub_t * out = (matteBytecodeStub_t*)matte_allocate(sizeof(matteBytecodeStub_t));
    out->isDynamicBinding = 0;
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

    matteValue_t dynamicBindTokenVal = matte_store_get_dynamic_bind_token(store);


    out->fileID = fileID;
    ADVANCE(uint32_t, out->stubID);
    ADVANCE(uint8_t, out->isVarArg);
    ADVANCE(uint8_t, out->argCount);
    out->argNames = (matteValue_t*)matte_allocate(sizeof(matteValue_t)*out->argCount);
    for(i = 0; i < out->argCount; ++i) {
        out->argNames[i] = chomp_string(store, bytes, left); 
        if (out->argNames[i].value.id == dynamicBindTokenVal.value.id) {
            out->isDynamicBinding = 1;
        }
    }        
    ADVANCE(uint8_t, out->localCount);
    out->localNames = (matteValue_t*)matte_allocate(sizeof(matteValue_t)*out->localCount);
    for(i = 0; i < out->localCount; ++i) {
        out->localNames[i] = chomp_string(store, bytes, left);    
    }        
    ADVANCE(uint32_t, out->stringCount);
    out->strings = (matteValue_t*)matte_allocate(sizeof(matteValue_t)*out->stringCount);
    for(i = 0; i < out->stringCount; ++i) {
        out->strings[i] = chomp_string(store, bytes, left);    
    }       
    ADVANCE(uint16_t, out->capturedCount);
    if (out->capturedCount) {
        out->captures = (matteBytecodeStubCapture_t*)matte_allocate(sizeof(matteBytecodeStubCapture_t)* out->capturedCount);
        ADVANCEN(sizeof(matteBytecodeStubCapture_t)*out->capturedCount, out->captures[0]);
    }
    ADVANCE(uint32_t, out->instructionCount);
    ADVANCE(uint32_t, out->startingLine);
    if (out->instructionCount) {
        out->instructions = (matteBytecodeStubInstruction_t*)matte_allocate(sizeof(matteBytecodeStubInstruction_t)* out->instructionCount);    
        for(i = 0; i < out->instructionCount; ++i) {
            ADVANCE(uint16_t, out->instructions[i].info.lineOffset);
            ADVANCE(uint8_t,  out->instructions[i].info.opcode);
            ADVANCE(double,   out->instructions[i].data);
        }
    }

    // complete linkage for NFN instructions
    // This will help other instances know the originating
    for(i = 0; i < out->instructionCount; ++i) {
        if (out->instructions[i].info.opcode == MATTE_OPCODE_NFN) {
            out->instructions[i].funcData.nfnFileID = fileID;
        }
    }
    return out;    
}

void matte_bytecode_stub_destroy(matteBytecodeStub_t * b) {
    matte_deallocate(b->strings);   
    matte_deallocate(b->captures);
    matte_deallocate(b->instructions);
    matte_deallocate(b->localNames);
    matte_deallocate(b->argNames);
    matte_deallocate(b);
}


matteArray_t * matte_bytecode_stubs_from_bytecode(
    matteStore_t * store,
    uint32_t fileID,
    const uint8_t * bytecodeRaw, 
    uint32_t len
) {
    matteArray_t * arr = matte_array_create(sizeof(matteBytecodeStub_t *));
    while(len) {
        matteBytecodeStub_t * s = bytes_to_stub(store, fileID, (uint8_t**)(&bytecodeRaw), &len);
        matte_array_push(arr, s);
    }
    return arr;
}



uint32_t matte_bytecode_stub_get_file_id(const matteBytecodeStub_t * stub) {
    return stub->fileID;
}

uint32_t matte_bytecode_stub_get_starting_line(const matteBytecodeStub_t * stub) {
    return stub->startingLine;
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

int matte_bytecode_stub_is_vararg(const matteBytecodeStub_t * stub) {
    return stub->isVarArg;
}


int matte_bytecode_stub_is_dynamic_bind(const matteBytecodeStub_t * stub) {
    return stub->isDynamicBinding;
}




