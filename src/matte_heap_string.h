#ifndef H_MATTE__STRING_HEAP__INCLUDED
#define H_MATTE__STRING_HEAP__INCLUDED

#include <stdint.h>
typedef struct matteString_t matteString_t;
typedef struct matteStringHeap_t matteStringHeap_t;

matteStringHeap_t * matte_string_heap_create();

void matte_string_heap_destroy(matteStringHeap_t *);

uint32_t matte_string_heap_internalize(matteStringHeap_t *, const matteString_t *);

// null if fail
const matteString_t * matte_string_heap_find(const matteStringHeap_t *, uint32_t);

#ifdef MATTE_DEBUG
void matte_string_heap_print(matteStringHeap_t * h);
#endif


#endif