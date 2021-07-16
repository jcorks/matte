/*
Copyright (c) 2021, Johnathan Corkery. (jcorkery@umich.edu)
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

#include <matte/compat.h>
#include <matte/containers/array.h>

#include <stdlib.h>
#include <string.h>



#define array_resize_amt 1.4
#define array_presize_amt 16

struct matteArray_t {
    uint32_t allocSize;
    uint32_t size;
    uint32_t sizeofType;
    uint8_t * data;
};


matteArray_t * matte_array_create(uint32_t typesize) {
    matteArray_t * a = malloc(sizeof(matteArray_t));
    a->sizeofType = typesize;
    a->size = 0;
    a->allocSize = array_presize_amt;
    a->data = malloc(typesize*array_presize_amt);
    return a;
}

void matte_array_destroy(matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    free(t->data);
    free(t);
}

const matteArray_t * matte_array_empty() {
    static matteArray_t empty = {};
    return &empty;
}

#define matte_array_temp_max_calls 128
static matteArray_t * tempVals[matte_array_temp_max_calls] = {0};
static int tempIter = 0;

const matteArray_t * matte_array_temporary_from_static_array(void * arr, uint32_t sizeofType, uint32_t len) {
    if (tempIter >= matte_array_temp_max_calls) tempIter = 0;
    if (tempVals[tempIter]) {
        matte_array_destroy(tempVals[tempIter]);
    }
    tempVals[tempIter] = matte_array_create(sizeofType);
    matte_array_push_n(tempVals[tempIter], arr, len);
    return tempVals[tempIter++];
}


matteArray_t * matte_array_clone(const matteArray_t * src) {
    #ifdef MATTE_DEBUG
        assert(src && "matteArray_t pointer cannot be NULL.");
    #endif
    matteArray_t * a = malloc(sizeof(matteArray_t));

    // do not clone pre-alloc size
    a->allocSize = src->size;
    a->size = src->size;
    a->sizeofType = src->sizeofType;
    a->data = malloc(src->size*a->sizeofType);
    memcpy(a->data, src->data, src->size*a->sizeofType);
    return a;
}

uint32_t matte_array_lower_bound(const matteArray_t * t, const void * ptrToEle, int(*comp)(const void * a, const void * b)) {
    int64_t lo = 0;
    int64_t hi = t->size;
    int64_t mid;
    while(lo < hi) {
        mid = lo + ((hi-lo) >> 1);
        void * val = t->data + t->sizeofType*mid;
        if (comp(ptrToEle, val)) {
            hi = mid;
        } else {
            lo = mid+1;
        }
    }

    return lo;
}

void matte_array_insert_n(matteArray_t * t, uint32_t index, void * ele, uint32_t count) {
    if (index >= t->size) {
        matte_array_push_n(t, ele, count);
        return;
    }
    matte_array_set_size(t, t->size+count);    
    
    memmove(
        t->data+(t->sizeofType*(index+count)),
        t->data+(t->sizeofType*index),
        t->sizeofType*(t->size-index)
    );

    memcpy(
        t->data+(t->sizeofType*index),
        ele,
        count*(t->sizeofType)
    );
}


uint32_t matte_array_get_size(const matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    return t->size;
}

uint32_t matte_array_get_type_size(const matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    return t->sizeofType;
}

void matte_array_push_n(matteArray_t * t, const void * elements, uint32_t count) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    while(t->size + count > t->allocSize) {
        t->allocSize += t->allocSize*array_presize_amt+1;
        t->data = realloc(t->data, t->allocSize*t->sizeofType);
    }
    memcpy(
        (t->data)+(t->size*t->sizeofType), 
        elements, 
        count*t->sizeofType
    );
    t->size+=count;
}


void matte_array_remove(matteArray_t * t, uint32_t index) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
        assert(index < t->size);
    #endif

    uint32_t indexByte = index*t->sizeofType;
    memmove(
        t->data+(indexByte),
        t->data+(indexByte + t->sizeofType),
        ((t->size-1)*t->sizeofType) - indexByte
    );
    t->size--;
}



void * matte_array_get_data(const matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    return t->data;
}


void matte_array_clear(matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    t->size = 0;
}

void matte_array_set_size(matteArray_t * t, uint32_t size) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    while(size >= t->allocSize) {
        t->allocSize += t->allocSize*array_presize_amt;
        t->data = realloc(t->data, t->allocSize*t->sizeofType);
    }
    t->size = size;
}
