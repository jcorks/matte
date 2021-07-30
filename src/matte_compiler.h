#ifndef H_MATTE__COMPILER__INCLUDED
#define H_MATTE__COMPILER__INCLUDED

#include <stdint.h>
#include "matte_string.h"
uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch)
);

#endif