#ifndef H_MATTE__COMPILER__INCLUDED
#define H_MATTE__COMPILER__INCLUDED

#include <stdint.h>
#include "matte_string.h"
// attempts to take UTF8 source and compile it into 
// bytecode.
uint8_t * matte_compiler_run(
    // raw source. Does not neet to be nul-terminated.
    const uint8_t * source, 
    // Length of source in bytes.
    uint32_t len,
    // Output length of the bytecode buffer.
    uint32_t * size,

    // The fileID to use for this source.
    uint32_t fileID,

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
    // raw source. Does not neet to be nul-terminated.
    const uint8_t * source, 
    // Length of source in bytes.
    uint32_t len,
    // Output length of the bytecode buffer.
    uint32_t * size,

    // The fileID to use for this source.
    uint32_t fileID,

    // If an error occurs, this function will be called detailing what 
    // went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),

    // the user data supplied to the on error callback.
    void * data
);



// Attempts to take the given source and do the first step of 
// compilation and print the results. This is useful for debugging.
// but not much else.
void matte_compiler_tokenize(
    // the source text.
    const uint8_t * source, 
    // The length of the source text.
    uint32_t len,
    // The fileID to use for this source.
    uint32_t fileID,

    // If an error occurs this function will be called detailing 
    // what went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    
    // the user data supplied to the on error callback.
    void * data

);


#endif
