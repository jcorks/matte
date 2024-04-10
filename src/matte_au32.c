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

#include "matte_au32.h"

#include <stdlib.h>
#include <string.h>
#ifdef MATTE_DEBUG
    #include <assert.h>
#endif
#include "matte.h"



static float get_resize(uint32_t size) {
    if (size < 10) return 2;
    if (size < 1000) return 1.3;
    return 1.1;    
}

#define array_presize_amt 1



matteAU32_t * matte_au32_create() {
    matteAU32_t * a = (matteAU32_t*)matte_allocate(sizeof(matteAU32_t));
    a->size = 0;
    a->allocSize = array_presize_amt;
    a->data = (uint32_t*)matte_allocate(sizeof(uint32_t)*array_presize_amt);
    return a;
}

void matte_au32_destroy(matteAU32_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteAU32_t pointer cannot be NULL.");
    #endif
    matte_deallocate(t->data);
    matte_deallocate(t);
}






void matte_au32_set_size(matteAU32_t * t, uint32_t size) {
    #ifdef MATTE_DEBUG
        assert(t && "matteAU32_t pointer cannot be NULL.");
    #endif
    if (size > t->allocSize) {
        uint32_t oldSize = t->allocSize;
        t->allocSize = t->allocSize*get_resize(t->allocSize)+1;
        uint32_t * newData = (uint32_t*)matte_allocate(t->allocSize*sizeof(uint32_t));
        memcpy(newData, t->data, oldSize*sizeof(uint32_t));
        matte_deallocate(t->data);
        t->data = newData;
    }
    t->size = size;
}
