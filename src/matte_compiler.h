#ifndef H_MATTE__COMPILER__INCLUDED
#define H_MATTE__COMPILER__INCLUDED

#include <stdint.h>

uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const char *, uint32_t)
);

#endif