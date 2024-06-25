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
#include "matte.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef MATTE_DEBUG__STORE
#include <assert.h>
#endif


typedef struct {
    matteString_t * str;
    uint32_t refs;
    uint32_t id;
} matteStringInfo_t;

struct matteStringStore_t{
    // C-string to id
    matteTable_t * strbufferToID;
    // matteStringInfo_t
    matteArray_t * strings;
    matteArray_t * deadIDs;
} ;


matteStringStore_t * matte_string_store_create() {
    matteStringStore_t * h = (matteStringStore_t*)matte_allocate(sizeof(matteStringStore_t));
    h->strings = matte_array_create(sizeof(matteStringInfo_t));
    h->strbufferToID = matte_table_create_hash_c_string();
    h->deadIDs = matte_array_create(sizeof(uint32_t));
    matteStringInfo_t dummy = {};
    matte_array_push(h->strings, dummy);
    return h;
}

uint32_t matte_string_store_ref(matteStringStore_t * h, const matteString_t * str) {
    return matte_string_store_ref_cstring(h, matte_string_get_c_str(str));
}
uint32_t matte_string_store_ref_cstring(matteStringStore_t * h, const char * strc) {
    uint32_t id = (uintptr_t) matte_table_find(h->strbufferToID, strc);
    if (id == 0) {
        matteStringInfo_t val = {};
        val.str = matte_string_create_from_c_str("%s", strc);
        val.refs = 0;

        if (h->deadIDs->size) {
            id = matte_array_at(h->deadIDs, uint32_t, h->deadIDs->size-1);
            matte_array_shrink_by_one(h->deadIDs);        
            val.id = id;            
            matte_array_at(h->strings, matteStringInfo_t, id) = val;
        } else {
            id = matte_array_get_size(h->strings);        
            val.id = id;
            matte_array_push(h->strings, val);
        }
        matte_table_insert(h->strbufferToID, strc, (void*)(uintptr_t)id);



    } 
    matte_array_at(h->strings, matteStringInfo_t, id).refs++;
    return id;
}

void matte_string_store_ref_id(matteStringStore_t * h, uint32_t id) {
    if (id >= matte_array_get_size(h->strings)) return;
    matte_array_at(h->strings, matteStringInfo_t, id).refs++;
}

void matte_string_store_unref(matteStringStore_t * h, uint32_t id) {
    if (id >= matte_array_get_size(h->strings)) {
        #ifdef MATTE_DEBUG__STORE
            assert(!"Invalid string unrefd");
        #endif
        return;
    }
    
        
    matteStringInfo_t * ref = &matte_array_at(h->strings, matteStringInfo_t, id);
    
    #ifdef MATTE_DEBUG__STORE
        assert(ref->refs);
    #endif

    ref->refs--;        
    if (ref->refs == 0) {
        #ifdef MATTE_DEBUG__STORE
            printf("STRING %d DONE\n", id);
        #endif    
        matte_table_remove(h->strbufferToID, matte_string_get_c_str(ref->str));
        matte_string_destroy(ref->str);
        ref->str = NULL;
        matte_array_push(h->deadIDs, id);
    }
}


void matte_string_store_destroy(matteStringStore_t * h) {
    uint32_t i;
    uint32_t len = matte_array_get_size(h->strings);
    for(i = 1; i < len; ++i) {
        matteString_t * s = matte_array_at(h->strings, matteStringInfo_t, i).str;
        if (s) matte_string_destroy(s);
    }
    matte_array_destroy(h->strings);
    matte_table_destroy(h->strbufferToID);
    matte_array_destroy(h->deadIDs);
    matte_deallocate(h);
}


const matteString_t * matte_string_store_find(const matteStringStore_t * h, uint32_t i) {
    if (i >= matte_array_get_size(h->strings)) return NULL;
    return matte_array_at(h->strings, matteStringInfo_t, i).str;
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
