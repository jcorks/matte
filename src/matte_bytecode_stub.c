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

#ifdef MATTE_DEBUG
    #include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_ITEM_COUNT 1024*1024*16



static uint8_t MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[MATTE_OPCODE_COUNT] = {
    0,//MATTE_OPCODE_NOP,
    1, //MATTE_OPCODE_PRF,
    0, //MATTE_OPCODE_NEM,
    2, //MATTE_OPCODE_NNM,
    1, //MATTE_OPCODE_NBL,
    1, //MATTE_OPCODE_NST,
    0, //MATTE_OPCODE_NOB,
    1, //MATTE_OPCODE_NFN,
    0, //MATTE_OPCODE_CAS,
    0, //MATTE_OPCODE_CAA,    
    1, //MATTE_OPCODE_CAL,
    2, //MATTE_OPCODE_ARF,
    1, //MATTE_OPCODE_OSN,
    0, //MATTE_OPCODE_OLK,
    0, //MATTE_OPCODE_OLB,
    1, //MATTE_OPCODE_OPR,
    1, //MATTE_OPCODE_EXT,
    1, //MATTE_OPCODE_POP,
    0, //MATTE_OPCODE_CPY,
    0, //MATTE_OPCODE_RET,
    1, //MATTE_OPCODE_SKP ,
    1, //MATTE_OPCODE_ASP,
    1, //MATTE_OPCODE_PNR,
    0, //MATTE_OPCODE_LST,
    1, //MATTE_OPCODE_PTO,
    1, //MATTE_OPCODE_SFS,
    1, //MATTE_OPCODE_QRY,
    1, //MATTE_OPCODE_SCA,
    1, //MATTE_OPCODE_SCO,
    0, //MATTE_OPCODE_SPA,
    0, //MATTE_OPCODE_SPO,
    0, //MATTE_OPCODE_OAS,
    0, //MATTE_OPCODE_LOP,
    0, //MATTE_OPCODE_FVR,
    0, //MATTE_OPCODE_FCH,
    1, //MATTE_OPCODE_CLV,
    0, //MATTE_OPCODE_NEF,
    0, //MATTE_OPCODE_PIP,    
};
static void matte_instruction_stream_decode(
    matteBytecodeStubInstruction_t * instructions,
    uint8_t options,
    uint32_t len,
    uint32_t * startLine,
    uint8_t ** bytesIn,
    uint32_t * leftIn
);


// Converts a raw instruction stream into a compressed form 
// used for raw bytecode stubs. The returned buffer must be 
// freed with matte_deallocate(). The length in bytes is returned.
static uint8_t * matte_instruction_stream_encode(
    matteBytecodeStubInstruction_t *,
    uint8_t options,
    uint32_t count,
    int includeDebug,
    uint32_t startingLine,
    uint32_t * len
);


// ASSUMES THAT STRING IDS AS GIVEN BY 
// THE STORE PERSIST THROUGHOUT THE STORE'S LIFETIME
// AS IN THE CURRENT IMPLEMENTATION. this will need 
// to change if the store string handling changes!


struct matteBytecodeStub_t {
    matteStore_t * store;
    
    // These are the string bases. After linking,
    // each set of strings is replaced by
    matteString_t ** protoArgs;
    matteString_t ** protoLocals;
    matteString_t ** protoStrings;

    uint32_t unitID;
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
    uint8_t options;

};  


matteBytecodeStub_t * matte_bytecode_stub_create(const matteBytecodeStubLayout_t * layout) {
    matteBytecodeStub_t * stub = matte_allocate(sizeof(matteBytecodeStub_t));
    stub->unitID = layout->unitID;
    stub->stubID = layout->stubID;
    
    if (layout->argumentNames) {
        stub->argCount = matte_array_get_size(layout->argumentNames);
        stub->protoArgs = (matteString_t**)layout->argumentNames->data;
        matte_array_destroy_xfer(layout->argumentNames);
    }
    
    if (layout->localNames) {
        stub->localCount = matte_array_get_size(layout->localNames);
        stub->protoLocals = (matteString_t**)layout->localNames->data;
        matte_array_destroy_xfer(layout->localNames);
    }
    
    if (layout->localStrings) {
        stub->stringCount = matte_array_get_size(layout->localStrings);
        stub->protoStrings = (matteString_t**)layout->localStrings->data;
        matte_array_destroy_xfer(layout->localStrings);
    }
    
    
    if (layout->captures) {    
        stub->capturedCount = matte_array_get_size(layout->captures);
        stub->captures = (matteBytecodeStubCapture_t*)layout->captures->data;
        matte_array_destroy_xfer(layout->captures);
    }
    
    if (layout->instructions) {
        stub->instructionCount = matte_array_get_size(layout->instructions);    
        stub->instructions = (matteBytecodeStubInstruction_t*) layout->instructions->data;
        matte_array_destroy_xfer(layout->instructions);    
    }
    
    stub->options = layout->options;
    stub->startingLine = layout->startingLine;
    return stub;
}




// prevents incomplete advances


#define ADVANCE(__T__, __V__) ADVANCE_SRC(sizeof(__T__), &(__V__), left, bytes);
#define ADVANCEN(__N__, __V__) ADVANCE_SRC(__N__, &(__V__), left, bytes);
#define UNROLL_UINT() UNROLL_UINT_SRC(left, bytes);

static void ADVANCE_SRC(int n, void * ptr, uint32_t * left, uint8_t ** bytes) {
    int32_t v = n <= *left ? n : *left;
    memcpy(ptr, *bytes, v);
    *left-=v;
    *bytes+=v;
}


static uint32_t UNROLL_UINT_SRC(uint32_t * left, uint8_t ** bytes) {
    uint8_t count = 0;
    ADVANCE(uint8_t, count);
    uint32_t out = 0;
    
    if (count & 0x01) {
        out = count >> 1;
    } else {
        count = count >> 1;
        switch(count) {
          case 1:
            ADVANCE(uint8_t, count);
            out = count;
            break;
            
          case 2: {
            uint16_t v = 0;
            ADVANCE(uint16_t, v);
            out = v;
            break;
          }

          case 3: {
            uint8_t v[3] = {};
            ADVANCEN(3, v);
            out = v[0] + 0xff*v[1] + 0xffff*v[2];
            break;
          }
          
          case 4:
            ADVANCE(uint32_t, out);
            break;
        }
    }
    
    return out;

}


static matteString_t * chomp_string(uint8_t ** bytes, uint32_t * left) {
    matteString_t * str;
    uint32_t len = UNROLL_UINT();
    
    if (len > *left) {
        len = *left;
    }
    uint8_t * utf8raw = (uint8_t*)matte_allocate((len+1)*1);
    ADVANCEN(len, utf8raw[0]);
    
    str = matte_string_create_from_c_str("%s", utf8raw);
    matte_deallocate(utf8raw);
    return str;
}





matteBytecodeStub_t * matte_bytecode_stub_create_symbolic() {
    matteBytecodeStub_t * out = (matteBytecodeStub_t*)matte_allocate(sizeof(matteBytecodeStub_t));
    out->options = MATTE_BYTECODE_STUB__OPTION__IS_DYNAMIC_BIND;
    return out;
}


static matteBytecodeStub_t * bytes_to_stub(uint8_t ** bytes, uint32_t * left) {
    matteBytecodeStub_t * out = (matteBytecodeStub_t*)matte_allocate(sizeof(matteBytecodeStub_t));
    uint32_t i;
    uint8_t ver = 0;

    uint8_t tag[6] = {};
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


    out->stubID = UNROLL_UINT();
    ADVANCE(uint8_t, out->options);
    ADVANCE(uint8_t, out->argCount);
    
    
    
    out->protoArgs = (matteString_t**)matte_allocate(sizeof(matteString_t*)*out->argCount);
    for(i = 0; i < out->argCount; ++i) {
        out->protoArgs[i] = chomp_string(bytes, left); 
    }        
    ADVANCE(uint8_t, out->localCount);
    out->protoLocals = (matteString_t**)matte_allocate(sizeof(matteString_t*)*out->localCount);
    for(i = 0; i < out->localCount; ++i) {
        out->protoLocals[i] = chomp_string(bytes, left);    
    }        
    out->stringCount = UNROLL_UINT();
    if (out->stringCount >= MAX_ITEM_COUNT) {
        *left = 0;
        return out;
    }

    out->protoStrings = (matteString_t**)matte_allocate(sizeof(matteString_t*)*out->stringCount);
    for(i = 0; i < out->stringCount; ++i) {
        out->protoStrings[i] = chomp_string(bytes, left);    
    }       
    out->capturedCount = UNROLL_UINT();
    if (out->capturedCount) {
        out->captures = (matteBytecodeStubCapture_t*)matte_allocate(sizeof(matteBytecodeStubCapture_t)* out->capturedCount);
        for(i = 0; i < out->capturedCount; ++i) {
            out->captures[i].stubID = UNROLL_UINT();
            out->captures[i].referrable = UNROLL_UINT();
        }
    }
    out->instructionCount = UNROLL_UINT();
    if (out->instructionCount >= MAX_ITEM_COUNT) {
        *left = 0;
        return out;
    }
    
    out->instructions = (matteBytecodeStubInstruction_t*)matte_allocate(sizeof(matteBytecodeStubInstruction_t)* out->instructionCount);    
    matte_instruction_stream_decode(
        out->instructions,
        out->options,
        out->instructionCount,
        &out->startingLine,
        bytes,
        left
    );
    
        /*
        for(i = 0; i < out->instructionCount; ++i) {
            ADVANCE(uint16_t, out->instructions[i].info.lineOffset);
            ADVANCE(uint8_t,  out->instructions[i].info.opcode);
            ADVANCE(double,   out->instructions[i].data);
        }
        */


    return out;    
}

void matte_bytecode_stub_destroy(matteBytecodeStub_t * b) {
    uint32_t i;
    if (b->protoArgs) {
        for(i = 0; i < b->argCount; ++i) {
            matte_string_destroy(b->protoArgs[i]);
        }
        matte_deallocate(b->protoArgs);
    } else {
        for(i = 0; i < b->argCount; ++i) {
            matte_store_recycle(b->store, b->argNames[i]);
        }    
        matte_deallocate(b->argNames);
    }
    

    if (b->protoLocals) {
        for(i = 0; i < b->localCount; ++i) {
            matte_string_destroy(b->protoLocals[i]);
        }
        matte_deallocate(b->protoLocals);    
    } else {
        for(i = 0; i < b->localCount; ++i) {
            matte_store_recycle(b->store, b->localNames[i]);
        }    
        matte_deallocate(b->localNames);
    }


    if (b->protoStrings) {
        for(i = 0; i < b->stringCount; ++i) {
            matte_string_destroy(b->protoStrings[i]);
        }
        matte_deallocate(b->protoStrings);    
    } else {
        for(i = 0; i < b->stringCount; ++i) {
            matte_store_recycle(b->store, b->strings[i]);
        }    
        matte_deallocate(b->strings);
    }


    matte_deallocate(b->captures);
    matte_deallocate(b->instructions);
    matte_deallocate(b);
}

void matte_bytecode_stub_link(matteBytecodeStub_t * stub, matteStore_t * store, uint32_t unitID) {
    stub->store = store;
    stub->unitID = unitID;

}



matteArray_t * matte_bytecode_stubs_decode_binary(
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


#define WRITE_BYTES(__T__, __VAL__) matte_array_push_n(byteout, &(__VAL__), sizeof(__T__));
#define WRITE_NBYTES(__N__, __VALP__) matte_array_push_n(byteout, (__VALP__), __N__);



static uint8_t * write_rolled_uint_bytes(uint8_t * iter, uint32_t val) {
    uint8_t needed;
    if (val <= 127) {
        needed = val;
        (*(iter++)) = (needed<<1) + 1;
    } else if (val <= 0xff) {
        needed = 2;
        uint8_t val8 = val;
        (*(iter++)) = needed;
        (*(iter++)) = val8;
    } else if (val <= 0xffff) {
        needed = 4;
        uint16_t val16 = val;
        (*(iter++)) = needed;
        (*(iter++)) = val16 % 0xff;
        (*(iter++)) = val16 / 0xff;
    } else if (val <= 0xffffff) {
        needed = 6;
        uint8_t val24[3] = {
            val % 0xff,
            (val >> 8) % 0xff,
            (val >> 16) % 0xff
        };
        (*(iter++)) = needed;
        (*(iter++)) = val24[0];
        (*(iter++)) = val24[1];
        (*(iter++)) = val24[2];
    } else {
        needed = 8;
        (*(iter++)) = needed;
        uint8_t * b = (uint8_t*)&val;
        (*(iter++)) = b[0];
        (*(iter++)) = b[1];
        (*(iter++)) = b[2];
        (*(iter++)) = b[3];
    }
    
    return iter;
}

static void write_rolled_uint_array(matteArray_t * byteout, uint32_t val) {
    uint8_t bytes[5];
    uint8_t * iter = write_rolled_uint_bytes(bytes, val);
    matte_array_push_n(byteout, bytes, iter - bytes);
}

static void write_unistring(matteArray_t * byteout, const matteString_t * str) {
    uint32_t len = matte_string_get_utf8_length(str);
    write_rolled_uint_array(byteout, len);
    WRITE_NBYTES(len, matte_string_get_utf8_data(str));
}




uint8_t * matte_bytecode_stubs_encode_binary(
    const matteArray_t * arr,
    uint32_t * size
) {
    *size = 0;
    matteArray_t * byteout = matte_array_create(sizeof(uint8_t));
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    uint32_t n;
    uint8_t nSlots;
    uint16_t nCaps;
    uint32_t nInst;
    uint32_t nStrings;

    uint8_t tag[] = {
        'M', 'A', 'T', 0x01, 0x06, 'B', 0x1
    };
    for(i = 0; i < len; ++i) {
        matteBytecodeStub_t * block = matte_array_at(arr, matteBytecodeStub_t *, i);

        nSlots = 1;
        WRITE_NBYTES(7, tag); // HEADER + bytecode version
        write_rolled_uint_array(byteout, block->stubID);
        nSlots = block->options;
        WRITE_BYTES(uint8_t, nSlots);

        nSlots = block->argCount;
        WRITE_BYTES(uint8_t, nSlots);

        if (block->protoArgs) {
            for(n = 0; n < nSlots; ++n) {
                write_unistring(byteout, block->protoArgs[n]);
            }
        } else {
            for(n = 0; n < nSlots; ++n) {
                write_unistring(
                    byteout, 
                    matte_value_string_get_string_unsafe(
                        block->store, 
                        block->argNames[n]
                    )
                );
            }        
        }

        nSlots = block->localCount;
        WRITE_BYTES(uint8_t, nSlots);
        if (block->protoLocals) {
            for(n = 0; n < nSlots; ++n) {
                write_unistring(byteout, block->protoLocals[n]);
            }
        } else {
            for(n = 0; n < nSlots; ++n) {
                write_unistring(
                    byteout, 
                    matte_value_string_get_string_unsafe(
                        block->store, 
                        block->localNames[n]
                    )
                );
            }        
        }


        nStrings = block->stringCount;
        write_rolled_uint_array(byteout, nStrings);
        if (block->protoStrings) {
            for(n = 0; n < nStrings; ++n) {
                write_unistring(byteout, block->protoStrings[n]);
            }
        } else {
            for(n = 0; n < nStrings; ++n) {
                write_unistring(
                    byteout, 
                    matte_value_string_get_string_unsafe(
                        block->store, 
                        block->strings[n]
                    )
                );
            }        
        }



        nCaps = block->capturedCount;
        write_rolled_uint_array(byteout, nCaps);
        //uint32_t szBefore = matte_array_get_size(byteout);
        for(n = 0; n < nCaps; ++n) {
            matteBytecodeStubCapture_t * cpt = block->captures+n;
            write_rolled_uint_array(byteout, cpt->stubID);
            write_rolled_uint_array(byteout, cpt->referrable);
        }
        //printf("Captures compressed from %'d to %'d bytes\n",
        //    (int)(nCaps * (sizeof(uint32_t) + sizeof(uint32_t))),
        //    (int)(matte_array_get_size(byteout) - szBefore)
        //);

        nInst = block->instructionCount;
        write_rolled_uint_array(byteout, nInst);

        
        uint32_t compressedSize = 0;
        uint8_t * compressed = matte_instruction_stream_encode(
            block->instructions,
            block->options,
            block->instructionCount,
            block->options & MATTE_BYTECODE_STUB__OPTION__DEBUG_INFO,
            block->startingLine,
            &compressedSize
        );
        
        WRITE_NBYTES(compressedSize, compressed);
        matte_deallocate(compressed);
        /*
        nInst = matte_array_get_size(block->instructions);
        WRITE_BYTES(uint32_t, nInst);
        WRITE_BYTES(uint32_t, block->startingLine);
        for(n = 0; n < nInst; ++n) {
            matteBytecodeStubInstruction_t * inst = &matte_array_at(block->instructions, matteBytecodeStubInstruction_t, n);
            WRITE_BYTES(uint16_t, inst->info.lineOffset);
            WRITE_BYTES(uint8_t, inst->info.opcode);
            WRITE_BYTES(double, inst->data);        
        }
        */
       
        
    }


    *size = matte_array_get_size(byteout);
    uint8_t * out = matte_array_get_data(byteout);
    matte_array_destroy_xfer(byteout);
    return out;
}



uint32_t matte_bytecode_stub_get_unit_id(const matteBytecodeStub_t * stub) {
    return stub->unitID;
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

static matteValue_t * link_strings(
    matteString_t ** strings,
    uint32_t count,
    matteStore_t * store
) {
    uint32_t i;
    matteValue_t * values = matte_allocate(sizeof(matteValue_t)*count);

    for(i = 0; i < count; ++i) {
        matteValue_t v = matte_store_new_value(store);
        matte_value_into_string(store, &v, strings[i]);
        matte_string_destroy(strings[i]);
        values[i] = v;
    }
    
    return values;
}


matteValue_t matte_bytecode_stub_get_arg_name_noref(matteBytecodeStub_t * stub, uint8_t i) {    
    if (i >= stub->argCount || stub->store == NULL) {
        matteValue_t v = {};
        return v;
    }
    
    if (stub->argNames == NULL) {
        stub->argNames = link_strings(stub->protoArgs, stub->argCount, stub->store);
        matte_deallocate(stub->protoArgs);
        stub->protoArgs = NULL;
    }
    return stub->argNames[i];
}

matteValue_t matte_bytecode_stub_get_local_name_noref(matteBytecodeStub_t * stub, uint8_t i) {
    if (i >= stub->localCount || stub->store == NULL) {
        matteValue_t v = {};
        return v;
    }
    if (stub->localNames == NULL) {
        stub->localNames = link_strings(stub->protoLocals, stub->localCount, stub->store);
        matte_deallocate(stub->protoLocals);
        stub->protoLocals = NULL;
    }

    return stub->localNames[i];
}

matteValue_t matte_bytecode_stub_get_string_noref(matteBytecodeStub_t * stub, uint32_t i) {
    if (i >= stub->stringCount || stub->store == NULL) {
        matteValue_t v = {};
        return v;
    }

    if (stub->strings == NULL) {
        stub->strings = link_strings(stub->protoStrings, stub->stringCount, stub->store);
        matte_deallocate(stub->protoStrings);
        stub->protoStrings = NULL;
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
    return stub->options & MATTE_BYTECODE_STUB__OPTION__IS_VAR_ARG;
}


int matte_bytecode_stub_is_dynamic_bind(const matteBytecodeStub_t * stub) {
    return stub->options & MATTE_BYTECODE_STUB__OPTION__IS_DYNAMIC_BIND;
}





uint8_t * matte_instruction_stream_encode(
    matteBytecodeStubInstruction_t * insts,
    uint8_t options,
    uint32_t count,
    int includeDebug,
    uint32_t startingLine,
    uint32_t * outLen
) {
    
    uint32_t i;
    uint32_t len = count;
    
    uint8_t  * instStream = (uint8_t*) matte_allocate((sizeof(uint16_t) + sizeof(uint64_t)) * len);
     int8_t  * debgStream = NULL;
    
    uint8_t  * instStreamIter = instStream;

    for(i = 0; i < len; ++i) {
        matteBytecodeStubInstruction_t * inst = insts+i;
        if (inst->info.opcode >= MATTE_OPCODE_COUNT) {
            *(instStreamIter++) = 0;
            
            #ifdef MATTE_DEBUG
                printf("matte_instruction_stream_encode(): unrecognized opcode %d. Replacing with NOP for safety. Your program is likely corrupt.\n", inst->info.opcode);
            #endif
            continue;
        }



        uint8_t bulkCount = MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[inst->info.opcode];


        
        // what????
        *(instStreamIter++) = inst->info.opcode;

        if (bulkCount == 1) {
            instStreamIter = write_rolled_uint_bytes(instStreamIter, inst->data32.slot0);
        } else if (bulkCount == 2) {
            memcpy(instStreamIter, &(inst->data64), bulkCount*sizeof(uint32_t));        
            instStreamIter += bulkCount*(sizeof(uint32_t));        
        } else {
            continue;
        }
    }

    uint32_t instSize = instStreamIter - instStream;
    uint32_t debgSize = 0;
  
    *outLen = instSize + sizeof(uint32_t);


    if (includeDebug) {
        debgStream = matte_allocate((sizeof(int8_t) + sizeof(int16_t))*len + sizeof(uint32_t));

        int8_t  * debgStreamIter = &debgStream[0];

        debgStreamIter = (uint8_t*)write_rolled_uint_bytes((uint8_t*)debgStreamIter, startingLine);

        uint16_t start = insts[0].info.lineOffset;
        uint16_t last  = start;
        for(i = 0; i < len; ++i) {
            matteBytecodeStubInstruction_t * inst = insts+i;
            uint16_t offset = inst->info.lineOffset;
            int16_t diff = i == 0 ? offset : offset - last;
            
            if (abs(diff) < 124) {
                *(debgStreamIter++) = (int8_t)diff;
            } else {
                *(debgStreamIter++) = 124;
                memcpy(debgStreamIter, &diff, sizeof(int16_t));
                debgStreamIter+=sizeof(int16_t);
            }
            last = offset;
        }    
        
        debgSize = debgStreamIter - debgStream;
        *outLen += (debgSize) + sizeof(uint32_t);
    }
    
    
    uint8_t * out = matte_allocate(*outLen);
    uint8_t * outIter = out;

    memcpy(outIter, &instSize,  sizeof(uint32_t)); outIter += sizeof(uint32_t);
    memcpy(outIter, instStream, instSize); outIter += instSize;
    
    if (includeDebug) {
        memcpy(outIter, &debgSize,  sizeof(uint32_t)); outIter += sizeof(uint32_t);
        memcpy(outIter, debgStream, debgSize); outIter += debgSize;
    }
    
    
    matte_deallocate(instStream);
    matte_deallocate(debgStream);
    
    //printf("Bytecode instruction stream compressed from %'d to %'d bytes\n",
    //    (int)(len * sizeof(matteBytecodeStubInstruction_t)),
    //    (int)*outLen
    //);
    
    return out;
}


void matte_instruction_stream_decode(
    matteBytecodeStubInstruction_t * instructions,
    uint8_t options,
    uint32_t len,
    uint32_t * startLine,
    uint8_t ** bytes,
    uint32_t * left
) {
    
    uint32_t i;
    
    
    uint32_t instStreamSize = 0;
    ADVANCE(uint32_t, instStreamSize);
    
    
    uint32_t n = 0;
    for(i = 0; i < instStreamSize; n++) {
        if (n >= len) break;
        ADVANCE(uint8_t, instructions[n].info.opcode); i+=1;
        
        if (instructions[n].info.opcode >= MATTE_OPCODE_COUNT) {
            #ifdef MATTE_DEBUG
                printf("matte_instruction_stream_decode(): unrecognized opcode %d. Replacing with NOP for safety. Your program is likely corrupt.\n", instructions[n].info.opcode);
            #endif
            
            instructions[n].info.opcode = MATTE_OPCODE_NOP;
            continue;
        }
                
        switch(MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[instructions[n].info.opcode]) {
          case 1:
            instructions[n].data32.slot0 = UNROLL_UINT();
            break;
            
          case 2:
            ADVANCEN(2*sizeof(uint32_t), instructions[n].data64); i+=2*sizeof(uint32_t);
            break;
                        
          case 0:
            break;
          default:
            instructions[n].info.opcode = MATTE_OPCODE_NOP;
            continue; // junk condition
        }
        

    }

    if (options & MATTE_BYTECODE_STUB__OPTION__DEBUG_INFO) {
        uint32_t debgStreamSize = 0;
        ADVANCE(uint32_t, debgStreamSize);
        
        
        *startLine = UNROLL_UINT();
        
        uint16_t current = 0;
        n = 0;
        for(i = 0; i < debgStreamSize; ++n) {
            if (n >= len) break;
            int8_t miniOffset = 0;
            ADVANCE(int8_t, miniOffset); i+=1;
            
            if (miniOffset == 124) {
                int16_t bigOffset;
                ADVANCE(uint16_t, bigOffset);i+=2;
                
                instructions[n].info.lineOffset = current + bigOffset;
            } else {
                instructions[n].info.lineOffset = current + miniOffset;
            }
            current = instructions[n].info.lineOffset;
        }
    
    }

}




