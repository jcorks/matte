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
#ifndef H_MATTE__COMPILER__INCLUDED
#define H_MATTE__COMPILER__INCLUDED

#include <stdint.h>
#include "matte_string.h"
typedef struct matteSyntaxGraph_t matteSyntaxGraph_t;


// attempts to take UTF8 source and compile it into 
// bytecode.
uint8_t * matte_compiler_run(
    // The cached syntax graph to use
    matteSyntaxGraph_t * graph,
    // raw source. Does not neet to be nul-terminated.
    const uint8_t * source, 
    // Length of source in bytes.
    uint32_t len,
    // Output length of the bytecode buffer.
    uint32_t * size,


    // If an error occurs, this function will be called detailing what 
    // went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),

    // the user data supplied to the on error callback.
    void * data
);


// Same as matte_compiler_run, except in any case there are 
// unfound refererables, instead of erroring out, it instead emits 
// PNR instructions, informing the VM to search for the reference 
// by named-string at runtime. This allows for compiling and running 
// actively during runtime to compute results (Just-In-Time compilation).
// Because of the reliance of named references, it is generally slower 
// and less reliable to use this method than normal compilation.
// Recall that referrable names are not guaranteed to be valid 
// and can be changed at any time.
uint8_t * matte_compiler_run_with_named_references(
    // The cached syntax graph to use
    matteSyntaxGraph_t * graph,
    // raw source. Does not neet to be nul-terminated.
    const uint8_t * source, 
    // Length of source in bytes.
    uint32_t len,
    // Output length of the bytecode buffer.
    uint32_t * size,

    // If an error occurs, this function will be called detailing what 
    // went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),

    // the user data supplied to the on error callback.
    void * data
);



// Attempts to take the given source and do the first step of 
// compilation and print the results. This is useful for debugging.
// but not much else.
matteString_t * matte_compiler_tokenize(
    // The cached syntax graph to use
    matteSyntaxGraph_t * graph,
    // the source text.
    const uint8_t * source, 
    // The length of the source text.
    uint32_t len,

    // If an error occurs this function will be called detailing 
    // what went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    
    // the user data supplied to the on error callback.
    void * data

);


#endif
