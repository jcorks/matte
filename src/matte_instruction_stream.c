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

/*

Stream compression is very simple and isnt really compression but 
a direct phase of translation into a CISC-like format, increasing code density.
In-memory it is expanded for simplicity, though.

The stream is split into 2 sections.

The first section is the compression of the instruction stream. This is 
laid out into memory as a stream of bytes, where each next byte is an instruction 
followed by a number of data bytes. Each instruction expects a certain number 
(0 to 2) of 4-byte blocks of input data to accompany the instruction.

The secton section is optional: it contains line offset data; in a similar manner 
it is "compressed" in a CISC-like format. following an initial uint16_t line offset from 
the bytecode stub root, each next byte determines offset that instruction has from 
the previous instruction (or if no previous instruction, the root offset). If the instruction 
changes a line count of more than 124, the value is set to 0xff and an additional 2 bytes
are written 

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "matte_instruction_stream.h"
#include "matte_string.h"
#include "matte_array.h"
#include "matte_opcode.h"
#include "matte.h"

#define ADVANCE(__T__, __V__) ADVANCE_SRC(sizeof(__T__), &(__V__), left, bytes);
#define ADVANCEN(__N__, __V__) ADVANCE_SRC(__N__, &(__V__), left, bytes);


uint8_t MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[MATTE_OPCODE_COUNT] = {
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

static void ADVANCE_SRC(int n, void * ptr, uint32_t * left, uint8_t ** bytes) {
    int32_t v = n <= *left ? n : *left;
    memcpy(ptr, *bytes, v);
    *left-=v;
    *bytes+=v;
}

static void write_rolled_uint(uint8_t ** iterIn, uint32_t val) {
    uint8_t * iter = *iterIn;
    uint8_t needed;
    if (val <= 0xff) {
        needed = 1;
        uint8_t val8 = val;
        *(iter++) = needed;
        *(iter++) = val8;
    } else if (val <= 0xffff) {
        needed = 2;
        uint16_t val16 = val;
        *(iter++) = needed;
        memcpy(iter, &val16, sizeof(uint16_t));
        iter += sizeof(uint16_t);
    } else if (val <= 0xffffff) {
        needed = 3;
        uint8_t val24[3] = {
            val % 0xff,
            (val >> 8) % 0xff,
            (val >> 16) % 0xff
        };
        *(iter++) = needed;
        memcpy(iter, &val24[0], 3);
        iter += 3;

    } else {
        needed = 4;
        *(iter++) = needed;
        memcpy(iter, &val, sizeof(uint32_t));
        iter += sizeof(uint32_t);
    }
    *iterIn = iter;
}


uint8_t * matte_instruction_stream_encode(
    matteArray_t * instSet,
    int includeDebug,
    uint32_t startingLine,
    uint32_t * outLen
) {
    
    uint32_t i;
    uint32_t len = matte_array_get_size(instSet);
    
    uint8_t  * instStream = (uint8_t*) matte_allocate((sizeof(uint16_t) + sizeof(uint64_t)) * len);
     int8_t  * debgStream = NULL;
    
    uint8_t  * instStreamIter = instStream;

    for(i = 0; i < len; ++i) {
        matteBytecodeStubInstruction_t * inst = &matte_array_at(instSet, matteBytecodeStubInstruction_t, i);
        uint8_t bulkCount = MATTE_INSTRUCTION_STREAM__OPCODE_TO_DATA_BLOCK_COUNT[inst->info.opcode];
        
        // what????
        *(instStreamIter++) = inst->info.opcode;

        if (bulkCount == 1) {
            write_rolled_uint(&instStreamIter, inst->data64);
        } else if (bulkCount == 2) {
            memcpy(instStreamIter, &(inst->data64), bulkCount*sizeof(uint32_t));        
            instStreamIter += bulkCount*(sizeof(uint32_t));        
        } else {
            continue;
        }
    }

    uint32_t instSize = instStreamIter - instStream;
    uint32_t debgSize = 0;
  
    *outLen = instSize + sizeof(uint32_t) + 1;


    if (includeDebug) {
        debgStream = matte_allocate((sizeof(int8_t) + sizeof(int16_t))*len + sizeof(uint32_t));

        int8_t  * debgStreamIter = debgStream;

        write_rolled_uint((uint8_t**)&debgStreamIter, startingLine);

        uint16_t start = matte_array_at(instSet, matteBytecodeStubInstruction_t, 0).info.lineOffset;
        uint16_t last  = start;
        for(i = 0; i < len; ++i) {
            matteBytecodeStubInstruction_t * inst = &matte_array_at(instSet, matteBytecodeStubInstruction_t, i);
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
    uint8_t options = 0;
    
    
    if (includeDebug) {
        options = MATTE_INSTRUCTION_STREAM__OPTIONS__DEBUG;
    }
    *(outIter++) = options;
    memcpy(outIter, &instSize,  sizeof(uint32_t)); outIter += sizeof(uint32_t);
    memcpy(outIter, instStream, instSize); outIter += instSize;
    
    if (includeDebug) {
        memcpy(outIter, &debgSize,  sizeof(uint32_t)); outIter += sizeof(uint32_t);
        memcpy(outIter, debgStream, debgSize); outIter += debgSize;
    }
    
    
    matte_deallocate(instStream);
    matte_deallocate(debgStream);
    
    printf("Bytecode instruction stream compressed from %'d to %'d bytes\n",
        (int)(len * sizeof(matteBytecodeStubInstruction_t)),
        (int)*outLen
    );
    
    return out;
}

#define UNROLL_UINT() UNROLL_UINT_SRC(&left, &bytes);

static uint32_t UNROLL_UINT_SRC(uint32_t ** leftIn, uint8_t *** bytesIn) {
    uint32_t * left = *leftIn;
    uint8_t ** bytes = *bytesIn;
    uint8_t count;
    ADVANCE(uint8_t, count);
    uint32_t out;
    switch(count) {
      case 1:
        ADVANCE(uint8_t, count);
        out = count;
        break;
        
      case 2: {
        uint16_t v;
        ADVANCE(uint16_t, v);
        out = v;
        break;
      }

      case 3: {
        uint8_t v[3];
        ADVANCEN(3, v);
        out = v[0] + 0xff*v[1] + 0xffff*v[2];
        break;
      }
      
      case 4:
        ADVANCE(uint32_t, out);
        break;
    }
    
    *leftIn = left;
    *bytesIn = bytes;
    return out;

}


void matte_instruction_stream_decode(
    matteBytecodeStubInstruction_t * instructions,
    uint32_t len,
    uint32_t * startLine,
    uint8_t *** bytesIn,
    uint32_t ** leftIn
) {
    uint8_t ** bytes = *bytesIn;
    uint32_t * left = *leftIn;
    
    uint32_t i;

    uint8_t options;
    ADVANCE(uint8_t, options);
    
    
    uint32_t instStreamSize;
    ADVANCE(uint32_t, instStreamSize);
    
    if (len > 0)
      printf("hi");
    
    uint32_t n = 0;
    for(i = 0; i < instStreamSize; n++) {
        if (n >= len) break;
        ADVANCE(uint8_t, instructions[n].info.opcode); i+=1;
                
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

    if (options & MATTE_INSTRUCTION_STREAM__OPTIONS__DEBUG) {
        uint32_t debgStreamSize;
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

    *bytesIn = bytes;
    *leftIn = left;
}



