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

    // If an error occurs, this function will be called detailing what 
    // went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch),
    // The fileID to use for this source.
    uint32_t fileID
);



// Attempts to take the given source and do the first step of 
// compilation and print the results. This is useful for debugging.
// but not much else.
void matte_compiler_tokenize(
    // the source text.
    const uint8_t * source, 
    // The length of the source text.
    uint32_t len,
    // If an error occurs this function will be called detailing 
    // what went wrong.
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch),
    // The fileID to use for this source.
    uint32_t fileID
);


#endif
