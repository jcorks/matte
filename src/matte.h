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
#ifndef H_MATTE__CONTEXT__INCLUDED
#define H_MATTE__CONTEXT__INCLUDED


typedef struct matte_t matte_t;
typedef struct matteVM_t matteVM_t;

#include "matte_heap.h"


// Creates a new Matte isntance
matte_t * matte_create();

// Destroys a Matte instance
void matte_destroy(matte_t *);





// Connects default IO wrappers for many of 
// Matte's VM features, making it much more 
// convenient to set up the most common 
// runtime options
void matte_set_io(
    matte_t *,
    // The function to handle needed input 
    // from the user. If no input is ready, NULL can be 
    // returned, and the input function will be called again.
    // If NULL, stdin is used.
    char * (*input) (matte_t *),       
    
    // The function to handle needed output to inform 
    // the user.
    // If NULL, stdout is used.    
    void   (*output)(matte_t *, const char *),
    
    // The function to clear output from Matte output.
    // If NULL, stdout is cleared using ANSI terminal characters.
    void   (*clear)(matte_t *);
);  



// When called, enables debugging features with default handlers
// using the IO functions within matte_set_io.
//
// When enabled, interfacing with debugging is similar to 
// GDB in style.
//
void matte_debugging_enable(matte_t *);


// Adds an external C function for use within the VM.
// This follows the semantics of matte_vm_set_external_function()
// but is more convenient.
//
// Optionally, a number of C Strings (UTF8 encoded) may be 
// specified as argument names. If undesired, the last argument should be 
// NULL. The number of names specified determines how many inputs 
// the function has, and consequently, how many values are 
// to be expected in the args array when the C function is valled.
void matte_add_external_function(
    matte_t *, 
    const char * name, 
    matteValue_t (*)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),
    void *,
    ... 
);


// Follows the semantics of matte_vm_call(), but is easier to 
// use. Additional args may be provided to bind parameters to their 
// respective names. A C String (UTF8 encoded) followed by a matteValue_t shall be provided if 
// desired. Regardless, the final argument must be NULL
matteValue_t matte_call(
    matte_t *, 
    matteValue_t func, 
    ... 
);


// Convenience function that runs the given source.
// Internally, it is given a unique name, compiled, and
// runs the new fileID immediately
matteValue_t matte_run_source(matte_t *, const char * source);


// Convenience function that compiles the source into bytecode.
uint8_t * matte_compile_source(matte_t *, uint32_t * bytecodeSize, const char * source);


// Convenience function that registers a module by-name with 
// the given bytecode
void matte_add_module(matte_t *, const char * name, const uint8_t * bytecode, uint32_t bytecodeSize);


// Convenience function that runs the given bytecode.
// Internally, it is given a unique name, compiled, and
// runs the new fileID immediately
matteValue_t matte_run_bytecode(matte_t *, const uint8_t * bytecode, uint32_t bytecodeSize);



// same as matte_run_bytecode, except with a parameters object.
matteValue_t matte_run_bytecode_with_parameters(matte_t *, const uint8_t * bytecode, uint32_t bytecodeSize, matteValue_t params);


// Same as matte_run_source, except with a parameters object
matteValue_t matte_run_source_with_parameters(matte_t *, const char * source, matteValue_t params);


// Nicely prints a value.
// not thread-safe.
const char * matte_introspect_value(matte_t *, matteValue_t);


// Gets generic data to be assigned to the matte instance
void * matte_get_user_data(matte_t * m);

// Sets generic data to be assigned to the matte instance
void matte_set_user_data(matte_t * m, void * d);


// Gets the VM instance from the matte instance.
// The VM is owned by the Matte instance.
matteVM_t * matte_get_vm(matte_t *);

#endif
