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
#include "matte_array.h"
#include "matte_bin.h"

#include <stdlib.h>
#include <string.h>

#ifdef MATTE_DEBUG
#include <assert.h>
#endif

struct matteBin_t {
    matteArray_t * alive;
    matteArray_t * dead;
    void * (*createNew)();
    void (*destroy)(void *);
};


typedef struct {
    uint32_t id;
    uint8_t * data;
} deadTag_t;

matteBin_t * matte_bin_create(void * (*createNew)(), void (*destroy)(void *)) {
    matteBin_t * out = (matteBin_t*)malloc(sizeof(matteBin_t));
    out->alive = matte_array_create(sizeof(void*));
    out->dead = matte_array_create(sizeof(deadTag_t));
    out->createNew = createNew;
    out->destroy = destroy;
    return out;
}

void matte_bin_destroy(matteBin_t * b) {
    uint32_t i;
    uint32_t len =         matte_array_get_size(b->alive);
    void ** objs = (void**)matte_array_get_data(b->alive);

    for(i = 0; i < len; ++i) {
        if (objs[i]) {
            b->destroy(objs[i]);
        }
    }
    len              =             matte_array_get_size(b->dead);
    deadTag_t * tags = (deadTag_t*)matte_array_get_data(b->dead);

    for(i = 0; i < len; ++i) {
        b->destroy(tags[i].data);
    }

    matte_array_destroy(b->alive);
    matte_array_destroy(b->dead);
    free(b);
}

void * matte_bin_add(matteBin_t * b, uint32_t * id) {
    uint32_t deadLen = matte_array_get_size(b->dead);
    void * obj;
    if (deadLen) {
        deadTag_t tag = matte_array_at(b->dead, deadTag_t, deadLen-1);
        matte_array_set_size(b->dead, deadLen-1);
        matte_array_at(b->alive, void *, tag.id) = tag.data;
        *id = tag.id;
        obj = tag.data;
    } else {
        *id = matte_array_get_size(b->alive);
        obj = b->createNew();
        matte_array_push(b->alive, obj);
    }
    return obj;
}

int matte_bin_contains(const matteBin_t * b, uint32_t id) {
    return matte_bin_fetch(b, id) != NULL;
}

void * matte_bin_fetch(const matteBin_t * b, uint32_t id) {
    if (id >= matte_array_get_size(b->alive)) return 0;
    return matte_array_at(b->alive, void*, id);
}

void matte_bin_recycle(matteBin_t * b, uint32_t id) {
    // not real
    if (id >= matte_array_get_size(b->alive)) return;
    // already removed
    if (matte_array_at(b->alive, void*, id) == NULL) return;

    deadTag_t tag;
    tag.id = id;
    tag.data = matte_array_at(b->alive, uint8_t*, id);
    matte_array_at(b->alive, void*, id) = NULL;
    matte_array_push(b->dead, tag);
}


const matteArray_t * matte_bin_get_active(const matteBin_t * b) {
    return b->alive;
}



