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

#include "matte_array.h"

#include <stdlib.h>
#include <string.h>
#ifdef MATTE_DEBUG
    #include <assert.h>
#endif
#include "matte.h"


#define array_resize_amt 1.3
#define array_presize_amt 1



matteArray_t * matte_array_create(uint32_t typesize) {
    matteArray_t * a = (matteArray_t*)matte_allocate(sizeof(matteArray_t));
    a->sizeofType = typesize;
    a->size = 0;
    a->allocSize = array_presize_amt;
    a->data = (uint8_t*)matte_allocate(typesize*array_presize_amt);
    return a;
}

void matte_array_destroy(matteArray_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    matte_deallocate(t->data);
    matte_deallocate(t);
}

const matteArray_t * matte_array_empty() {
    static matteArray_t empty = {};
    return &empty;
}


// For core library thread safety..... we are removing temporaries..


matteArray_t matte_array_temporary_from_static_array(void * data, uint32_t sizeofType, uint32_t len) {
    matteArray_t arr;
    arr.sizeofType = sizeofType;
    arr.size = len;
    arr.allocSize = len;
    arr.data = (uint8_t*)data;
    return arr;
}


matteArray_t * matte_array_clone(const matteArray_t * src) {
    #ifdef MATTE_DEBUG
        assert(src && "matteArray_t pointer cannot be NULL.");
    #endif
    matteArray_t * a = (matteArray_t*)matte_allocate(sizeof(matteArray_t));

    // do not clone pre-alloc size
    a->allocSize = src->size;
    a->size = src->size;
    a->sizeofType = src->sizeofType;
    a->data = (uint8_t*)matte_allocate(src->size*a->sizeofType);
    memcpy(a->data, src->data, src->size*a->sizeofType);
    return a;
}



void matte_array_insert_n(matteArray_t * t, uint32_t index, void * ele, uint32_t count) {
    if (index >= t->size) {
        matte_array_push_n(t, ele, count);
        return;
    }
    uint32_t tsize = t->size;
    matte_array_set_size(t, t->size+count);    
    
    memmove(
        t->data+(t->sizeofType*(index+count)),
        t->data+(t->sizeofType*index),
        t->sizeofType*(tsize-index)
    );

    memcpy(
        t->data+(t->sizeofType*index),
        ele,
        count*(t->sizeofType)
    );
}





void matte_array_push_n(matteArray_t * t, const void * elements, uint32_t count) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    while(t->size + count > t->allocSize) {
        uint32_t oldSize = t->allocSize*t->sizeofType;
        t->allocSize += t->allocSize*array_resize_amt+1;
        uint8_t * newData = (uint8_t*)matte_allocate(t->allocSize*t->sizeofType);
        memcpy(newData, t->data, oldSize);
        matte_deallocate(t->data);
        t->data = newData;
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


void matte_array_remove_n(matteArray_t * t, uint32_t index, uint32_t ct) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
        assert(index + ct < t->size);
    #endif

    uint32_t indexByte = index*t->sizeofType;
    memmove(
        t->data+(indexByte),
        t->data+(indexByte + ct*t->sizeofType),
        ((t->size - ct)*t->sizeofType) - indexByte
    );
    t->size-=ct;
}






void matte_array_set_size(matteArray_t * t, uint32_t size) {
    #ifdef MATTE_DEBUG
        assert(t && "matteArray_t pointer cannot be NULL.");
    #endif
    while(size >= t->allocSize) {
        uint32_t oldSize = t->allocSize*t->sizeofType;
        t->allocSize += t->allocSize*array_resize_amt;
        uint8_t * newData = (uint8_t*)matte_allocate(t->allocSize*t->sizeofType);
        memcpy(newData, t->data, oldSize);
        matte_deallocate(t->data);
        t->data = newData;
    }
    t->size = size;
}
