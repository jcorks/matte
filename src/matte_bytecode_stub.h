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





typedef struct {
    /// Line offset for the instruction from the stub function
    uint16_t lineOffset;
    /// The opcode;
    uint8_t opcode;
} matteBytecodeStubInstruction_Info_t;




/// Get all the stub's instructions
typedef struct {    
    /// information on the instruction;
    matteBytecodeStubInstruction_Info_t info;

    // auxiliary data
    union {
        uint64_t data64;
        struct {
            uint32_t slot0;
            uint32_t slot1;
        } data32;
    
        struct {
            uint16_t slot0;
            uint16_t slot1;
            uint16_t slot2;
            uint16_t slot3;
        } args;
        double number;
        struct {
            uint32_t stubID;
            /// Per-opcode auxiliary data. Currently only used for fileID for nfn opcodes
            uint32_t aux;
        } funcData;
    };

} matteBytecodeStubInstruction_t;


#include "matte_store.h"

typedef struct matteArray_t matteArray_t;
typedef struct matteString_t matteString_t;



/// Bytecode Stubs
///
/// Bytecode stubs represent compiled entities of Matte source.
/// They behave as a function, but may capture values from nearby functions.
///

typedef struct matteBytecodeStub_t matteBytecodeStub_t;

/// Options
enum {
    MATTE_BYTECODE_STUB__OPTION__IS_VAR_ARG = 1,
    MATTE_BYTECODE_STUB__OPTION__IS_DYNAMIC_BIND = 2,
    MATTE_BYTECODE_STUB__OPTION__DEBUG_INFO = 4
};


typedef struct {
    // The fileID of the bytecode stub
    uint32_t fileID;
    
    // Gets the ID of the stub within the fileID
    uint32_t stubID;
    
    // The starting line in source for the stub.
    uint32_t startingLine;
    
    // Options for the bytecode stub
    int options;
    
    
    
    // matteString_t *
    // All the argument names, in order.
    matteArray_t * argumentNames;
    
    // All the local names, in order,
    matteArray_t * localNames;
    
    // All the local strings, in order
    matteArray_t * localStrings;
    
    
    
    
    // matteBytecodeStubCapture_t
    // All captures for the stub.
    matteArray_t * captures;

    // matteBytecodeStubInstruction_t
    // All instructions for the stub
    matteArray_t * instructions;

} matteBytecodeStubLayout_t;


// Creates a new bytecode stub using a layout.
// NOTE: all matteArrays and matteStrings transfer ownership to this function 
// and should not be freed.
matteBytecodeStub_t * matte_bytecode_stub_create(const matteBytecodeStubLayout_t *);

/// Creates a symbolic bytecode stub that doesnt do anything 
/// and accepts all arguments.
matteBytecodeStub_t * matte_bytecode_stub_create_symbolic();

/// Generates an array of bytecode stubs (matteBytecodeStub_t *) from 
/// raw bytecode binary. If an error occurs, err is populated and NULL is returned.
/// When done, each matteBytecodeStub_t * should be removed.
///
/// Incomplete stubs are supported. If given, remaining attributes are 0.
matteArray_t * matte_bytecode_stubs_decode_binary(
    const uint8_t * bytecodeRaw, 
    uint32_t len
);

/// Encodes an array of bytecode stubs into 
/// a single binary encoding block.
uint8_t * matte_bytecode_stubs_encode_binary(
    // array of matteBytecodeStub_t *
    const matteArray_t * stubs,
    uint32_t * len
);

// Anchors the bytecode stub to a matteStore instance.
// Needs to be called before retrieving matteValue_t 
// values from the stub.
void matte_bytecode_stub_link(matteBytecodeStub_t *, matteStore_t *, uint32_t fileID);

/// Destroys a bytecode stub.
void matte_bytecode_stub_destroy(matteBytecodeStub_t * b);

/// Returns the fileid that the stub was given.
/// fileids are only valid if all stubs come from the same parser 
/// instance.
uint32_t matte_bytecode_stub_get_file_id(const matteBytecodeStub_t *);

/// Gets the line that this function started at in source.
uint32_t matte_bytecode_stub_get_starting_line(const matteBytecodeStub_t *);

/// Gets the id, local to the file.
/// stub ids are only valid if all stubs come from the same parser 
/// instance.
uint32_t matte_bytecode_stub_get_id(const matteBytecodeStub_t *);


/// Gets the count of arguments to this function.
uint8_t matte_bytecode_stub_arg_count(const matteBytecodeStub_t *);

/// Gets the count of local variables to this function.
uint8_t matte_bytecode_stub_local_count(const matteBytecodeStub_t *);


/// Gets the argument name of the local variable at the given index
/// If none, empty is returned
/// The returned string is NOT refd, so an additional unref is not needed.
matteValue_t matte_bytecode_stub_get_arg_name_noref(matteBytecodeStub_t *, uint8_t);

/// Gets the local variable name of the local variable at the given index
/// If none, empty is returned.
/// The returned string is NOT refd, so an additional unref is not needed.
matteValue_t matte_bytecode_stub_get_local_name_noref(matteBytecodeStub_t *, uint8_t);

/// Gets the pre-compiled static string by local ID.
/// These local IDs are used for NST
/// The returned string is NOT refd, so an additional unref is not needed.
matteValue_t matte_bytecode_stub_get_string_noref(matteBytecodeStub_t *, uint32_t localStringID);

/// Returns whether the function is a vararg function, meaning it will 
/// always have a single argument that contains all called arguments 
/// packed within it as an object.
int matte_bytecode_stub_is_vararg(const matteBytecodeStub_t *);


/// Returns whether the bytecode is dynamically bound, meaning 
/// an argument is named "$"
int matte_bytecode_stub_is_dynamic_bind(const matteBytecodeStub_t *);

/// ID of a stub capture variable
typedef struct {
    // stubID of the function that contains the captures.
    // during runtime, the stack will be walked for the nearest 
    // context that matches and will pull referrables from it.
    uint32_t stubID;

    // ID within that function scope where the variable referred to is.
    uint32_t referrable;
} matteBytecodeStubCapture_t;

/// Get all the external stub captures registered to this stub.
const matteBytecodeStubCapture_t * matte_bytecode_stub_get_captures(
    const matteBytecodeStub_t *, 
    uint32_t * count
);

/// Gets all instructions held by the stub.
const matteBytecodeStubInstruction_t * matte_bytecode_stub_get_instructions(const matteBytecodeStub_t *, uint32_t * count);



#endif
