/*
Copyright (c) 2021, Johnathan Corkery. (jcorkery@umich.edu)
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

#ifndef H_MATTE__BYTECODE_STUB__INCLUDED
#define H_MATTE__BYTECODE_STUB__INCLUDED

#include <stdint.h>
typedef struct matteArray_t matteArray_t;
typedef struct matteString_t matteString_t;





typedef struct matteBytecodeStub_t matteBytecodeStub_t;

// Generates an array of bytecode stubs (matteBytecodeStub_t *) from 
// raw bytecode. If an error occurs, err is populated and NULL is returned.
// When done, each matteBytecodeStub_t * should be removed.
//
// Incomplete stubs are supported. If given, remaining attributes are 0.
matteArray_t * matte_bytecode_stubs_from_bytecode(
    const uint8_t * bytecodeRaw, 
    uint32_t len
);

// free
void matte_bytecode_stub_destroy(matteBytecodeStub_t * b);

// Returns the fileid that the stub was given.
// fileids are only valid if all stubs come from the same parser 
// instance.
uint16_t matte_bytecode_stub_get_file_id(const matteBytecodeStub_t *);

// Gets the id, local to the file.
// stub ids are only valid if all stubs come from the same parser 
// instance.
uint16_t matte_bytecode_stub_get_id(const matteBytecodeStub_t *);


// Gets the count of arguments to this function.
uint8_t matte_bytecode_stub_arg_count(const matteBytecodeStub_t *);

// Gets the count of local variables to this function.
uint8_t matte_bytecode_stub_local_count(const matteBytecodeStub_t *);


// Gets the argument name of the local variable at the given index
// If none, NULL is returned.
const matteString_t * matte_bytecode_stub_get_arg_name(const matteBytecodeStub_t *, uint8_t);

// Gets the local variable name of the local variable at the given index
// If none, NULL is returned.
const matteString_t * matte_bytecode_stub_get_local_name(const matteBytecodeStub_t *, uint8_t);





// ID of a stub capture variable
typedef struct {
    // stubID of the function that contains the captures.
    // during runtime, the stack will be walked for the nearest 
    // context that matches and will pull referrables from it.
    uint16_t stubID;

    // ID within that function scope where the variable referred to is.
    uint32_t referrable;
} matteBytecodeStubCapture_t;

// Get all the external stub captures registered to this stub.
const matteBytecodeStubCapture_t * matte_bytecode_stub_get_captures(
    const matteBytecodeStub_t *, 
    uint32_t * count
);

// Get all the stub's instructions
typedef struct {
    // Line number for the parsed instruction
    int32_t lineNumber;
    // The opcode of the 
    int32_t  opcode;
    uint8_t  data[8];
} matteBytecodeStubInstruction_t;

// Gets all instructions held by the stub.
const matteBytecodeStubInstruction_t * matte_bytecode_stub_get_instructions(const matteBytecodeStub_t *, uint32_t * count);



#endif
