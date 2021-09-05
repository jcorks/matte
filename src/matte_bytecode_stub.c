#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_array.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct matteBytecodeStub_t {
    uint32_t fileID;
    uint32_t stubID;
    
    matteString_t ** argNames;
    matteString_t ** localNames;
    matteString_t ** strings;

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


static matteString_t * chomp_string(uint8_t ** bytes, uint32_t * left) {
    matteString_t * str = matte_string_create();
    uint32_t len = 0;
    ADVANCE(uint32_t, len);
    int32_t ch = 0;
    uint32_t i;
    for(i = 0; i < len; ++i) {
        ADVANCE(int32_t, ch);
        matte_string_append_char(str, ch);
    }
    return str;
}


static matteBytecodeStub_t * bytes_to_stub(uint8_t ** bytes, uint32_t * left) {
    matteBytecodeStub_t * out = calloc(1, sizeof(matteBytecodeStub_t));
    uint32_t i;
    uint8_t ver = 0;
    ADVANCE(uint8_t, ver);
    if (ver != 1) return out;
    ADVANCE(uint32_t, out->fileID);
    ADVANCE(uint32_t, out->stubID);
    ADVANCE(uint8_t, out->argCount);
    out->argNames = malloc(sizeof(matteString_t *)*out->argCount);
    for(i = 0; i < out->argCount; ++i) {
        out->argNames[i] = chomp_string(bytes, left);    
    }        
    ADVANCE(uint8_t, out->localCount);
    out->localNames = malloc(sizeof(matteString_t *)*out->localCount);
    for(i = 0; i < out->localCount; ++i) {
        out->localNames[i] = chomp_string(bytes, left);    
    }        
    ADVANCE(uint32_t, out->stringCount);
    out->strings = malloc(sizeof(matteString_t *)*out->stringCount);
    for(i = 0; i < out->stringCount; ++i) {
        out->strings[i] = chomp_string(bytes, left);    
    }       
    ADVANCE(uint16_t, out->capturedCount);
    if (out->capturedCount) {
        out->captures = calloc(sizeof(matteBytecodeStubCapture_t), out->capturedCount);
        ADVANCEN(sizeof(matteBytecodeStubCapture_t)*out->capturedCount, out->captures[0]);
    }
    ADVANCE(uint32_t, out->instructionCount);
    if (out->instructionCount) {
        out->instructions = calloc(sizeof(matteBytecodeStubInstruction_t), out->instructionCount);    
        ADVANCEN(sizeof(matteBytecodeStubInstruction_t)*out->instructionCount, out->instructions[0]);
    }
    return out;    
}

void matte_bytecode_stub_destroy(matteBytecodeStub_t * b) {
    uint32_t i;
    for(i = 0; i < b->argCount; ++i) {
        matte_string_destroy(b->argNames[i]);
    }     
    free(b->argNames);   
    for(i = 0; i < b->localCount; ++i) {
        matte_string_destroy(b->localNames[i]);
    }     
    free(b->localNames);   
    for(i = 0; i < b->stringCount; ++i) {
        matte_string_destroy(b->strings[i]);
    }     
    free(b->strings);   
    free(b->captures);
    free(b->instructions);
    free(b);
}



matteArray_t * matte_bytecode_stubs_from_bytecode(
    const uint8_t * bytecodeRaw, 
    uint32_t len
) {
    matteArray_t * arr = matte_array_create(sizeof(matteBytecodeStub_t *));
    while(len) {
        matteBytecodeStub_t * s = bytes_to_stub((uint8_t**)(&bytecodeRaw), &len);
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


const matteString_t * matte_bytecode_stub_get_arg_name(const matteBytecodeStub_t * stub, uint8_t i) {
    if (i >= stub->argCount) return NULL;
    return stub->argNames[i];
}

const matteString_t * matte_bytecode_stub_get_local_name(const matteBytecodeStub_t * stub, uint8_t i) {
    if (i >= stub->localCount) return NULL;
    return stub->localNames[i];
}

const matteString_t * matte_bytecode_stub_get_string(const matteBytecodeStub_t * stub, uint32_t i) {
    if (i >= stub->stringCount) return NULL;
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





