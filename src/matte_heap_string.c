#include "matte_heap_string.h"
#include "matte_string.h"
#include "matte_table.h"
#include "matte_string.h"
#include "matte_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct matteStringHeap_t{
    matteTable_t * strbufferToID;
    matteArray_t * strings;
} ;


matteStringHeap_t * matte_string_heap_create() {
    matteStringHeap_t * h = calloc(1, sizeof(matteStringHeap_t));
    h->strings = matte_array_create(sizeof(matteString_t *));
    h->strbufferToID = matte_table_create_hash_c_string();
    matteString_t * str = NULL;
    matte_array_push(h->strings, str);
    return h;
}

uint32_t matte_string_heap_internalize(matteStringHeap_t * h, const matteString_t * str) {
    uint32_t id = (uint32_t) matte_table_find(h->strbufferToID, matte_string_get_c_str(str));
    if (id == 0) {
        matteString_t * interned = matte_string_clone(str);
        id = matte_array_get_size(h->strings);
        matte_array_push(h->strings, interned);
        matte_table_insert(h->strbufferToID, matte_string_get_c_str(interned), (void*)id);
        return id;
    } else {
        return id;
    }
}
uint32_t matte_string_heap_internalize_cstring(matteStringHeap_t * h, const char * strc) {
    matteString_t * str = matte_string_create_from_c_str(strc);
    uint32_t id = (uint32_t) matte_table_find(h->strbufferToID, matte_string_get_c_str(str));
    if (id == 0) {
        matteString_t * interned = str;
        id = matte_array_get_size(h->strings);
        matte_array_push(h->strings, interned);
        matte_table_insert(h->strbufferToID, matte_string_get_c_str(interned), (void*)id);
        return id;
    } else {
        matte_string_destroy(str);
        return id;
    }
}

void matte_string_heap_destroy(matteStringHeap_t * h) {
    uint32_t i;
    uint32_t len = matte_array_get_size(h->strings);
    for(i = 1; i < len; ++i) {
        matteString_t * s = matte_array_at(h->strings, matteString_t *, i);
        matte_string_destroy(s);
    }
    matte_array_destroy(h->strings);
    matte_table_destroy(h->strbufferToID);
    free(h);
}


const matteString_t * matte_string_heap_find(const matteStringHeap_t * h, uint32_t i) {
    if (i >= matte_array_get_size(h->strings)) return NULL;
    return matte_array_at(h->strings, matteString_t *, i);
}

#ifdef MATTE_DEBUG
void matte_string_heap_print(matteStringHeap_t * h) {
    uint32_t i;
    uint32_t len = matte_array_get_size(h->strings);
    for(i = 0; i < len; ++i) {
        printf("%d  | %s\n", i, matte_string_get_c_str(matte_array_at(h->strings, matteString_t *, i)));
    }
}
#endif
