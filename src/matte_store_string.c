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
#include "matte_store_string.h"
#include "matte_string.h"
#include "matte_table.h"
#include "matte_string.h"
#include "matte_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct matteStringStore_t{
    matteTable_t * strbufferToID;
    matteArray_t * strings;
} ;


matteStringStore_t * matte_string_store_create() {
    matteStringStore_t * h = (matteStringStore_t*)calloc(1, sizeof(matteStringStore_t));
    h->strings = matte_array_create(sizeof(matteString_t *));
    h->strbufferToID = matte_table_create_hash_c_string();
    matteString_t * str = NULL;
    matte_array_push(h->strings, str);
    return h;
}

uint32_t matte_string_store_internalize(matteStringStore_t * h, const matteString_t * str) {
    uint32_t id = (uintptr_t) matte_table_find(h->strbufferToID, matte_string_get_c_str(str));
    if (id == 0) {
        matteString_t * interned = matte_string_clone(str);
        id = matte_array_get_size(h->strings);
        matte_array_push(h->strings, interned);
        matte_table_insert(h->strbufferToID, matte_string_get_c_str(interned), (void*)(uintptr_t)id);
        return id;
    } else {
        return id;
    }
}
uint32_t matte_string_store_internalize_cstring(matteStringStore_t * h, const char * strc) {
    matteString_t * str = matte_string_create_from_c_str(strc);
    uint32_t id = (uintptr_t) matte_table_find(h->strbufferToID, matte_string_get_c_str(str));
    if (id == 0) {
        matteString_t * interned = str;
        id = matte_array_get_size(h->strings);
        matte_array_push(h->strings, interned);
        matte_table_insert(h->strbufferToID, matte_string_get_c_str(interned), (void*)(uintptr_t)id);
        return id;
    } else {
        matte_string_destroy(str);
        return id;
    }
}

void matte_string_store_destroy(matteStringStore_t * h) {
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


const matteString_t * matte_string_store_find(const matteStringStore_t * h, uint32_t i) {
    if (i >= matte_array_get_size(h->strings)) return NULL;
    return matte_array_at(h->strings, matteString_t *, i);
}

#ifdef MATTE_DEBUG
void matte_string_store_print(matteStringStore_t * h) {
    uint32_t i;
    uint32_t len = matte_array_get_size(h->strings);
    for(i = 0; i < len; ++i) {
        matteString_t * str = matte_array_at(h->strings, matteString_t *, i);
        if (str)
            printf("%d  | %s\n", i, matte_string_get_c_str(str));
    }
}
#endif
