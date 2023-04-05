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
void matte_use_default_io(
    // The function to handle needed input 
    // from the user. If no input is ready, NULL can be 
    // returned, and the input function will be called again.
    // If NULL, stdin is used.
    char * (*input) (),       
    
    // The function to handle needed output to inform 
    // the user.
    // If NULL, stdout is used.    
    void   (*output)(const char *),

    // whether to enable debugging in the VM. If enabled, 
    // the matte instance will provide default debug handlers    
    int isDebug
);  


// Adds an external C function for use within the VM.
// This follows the semantics of matte_vm_set_external_function()
// but is more convenient.
void matte_add_external_function(
    // The embedding
    matte_t *, 
    const char * name, 
    matteValue_t (*)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),
    void *,
    ... 
);

// Convenience function that runs the given source or bytecode.
// Internally, it is given a unique name, compiled (if detected as needed), and
// runs the new fileID immediately
matteValue_t matte_run_source(matte_t *, const char * source);


// Nicely prints a value.
const char * matte_print_value(matte_t *, matteValue_t);




matteVM_t * matte_get_vm(matte_t *);

#endif
