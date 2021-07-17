#include "matte_bytecode_stub.h"

struct matteBytecodeStub_t {
    uint16_t fileID;
    uint16_t stubID;
    
    matteString_t ** argNames;
    matteString_t ** localNames;
    matteBytecodeStubCapture_t ** captures; 
    matteBytecodeStubInstruction_t * instructions;

    uint8_t argCount;
    uint8_t localCount;
    uint16_t capturedCount;
    uint32_t instructionCount;
};  

// prevents incomplete advances
#define ADVANCE(__T__, __v__) {uint32_t v__ = sizeof(__T__) <= *left ? sizeof(__T__); memcpy(&(__v__), *bytes, v__); *left-=v__;*bytes+=v__}
#define ADVANCEN(__N__, __v__) {uint32_t v__ = __N__ <= *left ? __N__; memcpy(&(__v__), *bytes, v__); *left-=v__; *bytes+=v__}

static matteString_t * chomp_string(uint8_t ** bytes, uint32_t * left) {
    matteString_t * str = matte_string_create();
    uint32_t len;
    ADVANCE(uint32_t, len);
    uint8_t * rawBytes = calloc(len,1);
    if (rawBytes) {
        rawBytes[len] = 0;
        ADVANCEN(len, rawBytes);
        // TODO: UTF8
        matte_string_concat_printf(str, rawBytes);
    }
    return str;
}


static matteBytecodeStub_t * bytes_to_stub(uint8_t ** bytes, uint32_t * left) {
    matteBytecodeStub_t * out = calloc(1, sizeof(matteBytecodeStub_t));
    uint32_t i;
    
    ADVANCE(uint16_t, out->fileID);
    ADVANCE(uint16_t, out->stubID);
    ADVANCE(uint8_t, out->argCount);
    out->argNames = sizeof(matteString_t *);
    for(i = 0; i < out->argCount; ++i) {
        out->argNames[i] = chomp_string(bytes, left);    
    }        
    ADVANCE(uint8_t, out->localCount);
    out->localNames = sizeof(matteString_t *);
    for(i = 0; i < out->localCount; ++i) {
        out->localNames[i] = chomp_string(bytes, left);    
    }        
    ADVANCE(uint16_t, out->capturedCount);
    out->captures = calloc(sizeof(matteBytecodeStubCapture_t), out->capturedCount);
    ADVANCEN(sizeof(matteBytecodeStubCapture_t)*out->capturedCount, out->captures);

    ADVANCE(uint32_t, out->instructionCount);
    out->captures = calloc(sizeof(matteBytecodeStubInstruction_t, out->instructionCount);    
    ADVANCEN(sizeof(matteBytecodeStubInstruction_t)*out->captureCount, out->instructions);

    return out;    
}



matteArray_t * matte_bytecode_stubs_from_bytecode(
    const uint8_t * bytecodeRaw, 
    uint32_t len, 
    matteError_t * err
) {
    matteArray_t * arr = matte_array_create(sizeof(matteBytecodeStub_t *));
    while(len) {
        matteBytecodeStub_t * s = bytes_to_stub((uint8_t**)(&bytecodeRaw), &len);
        matte_array_push(arr, s);
    }
    return arr;
}



uint16_t matte_bytecode_stub_get_file_id(const matteBytecodeStub_t * stub) {
    return stub->fileID;
}

uint16_t matte_bytecode_stub_get_id(const matteBytecodeStub_t * stub) {
    return stub->stubID;
}


uint8_t matte_bytecode_stub_arg_count(const matteBytecodeStub_t * stub) {
    return stub->argCount;
}


uint8_t matte_bytecode_stub_local_count(const matteBytecodeStub_t * stub) {
    return stub->localCountl
}


const matteString_t * matte_bytecode_stub_get_arg_name(const matteBytecodeStub_t * stub, uint8_t i) {
    if (i >= stub->argCount) return NULL;
    return stub->argNames[i];
}

const matteString_t * matte_bytecode_stub_get_local_name(const matteBytecodeStub_t *, uint8_t) {
    if (i >= stub->localCount) return NULL;
    return stub->localNames[i];
}






