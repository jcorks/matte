/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
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


#ifndef H_MATTE__POOL__INCLUDED
#define H_MATTE__POOL__INCLUDED


#include <stdint.h>
#include "matte_array.h"
#include "matte_au32.h"


typedef struct mattePool_t mattePool_t;



mattePool_t * matte_pool_create(uint32_t sizeofType, void (*)(void *));

// Destroys a node pool
void matte_pool_destroy(mattePool_t * m);


// Creates a new node from the pool
uint32_t matte_pool_add(mattePool_t * m);

uint8_t * matte_pool_get_all(mattePool_t * m, uint32_t *);

// destroys a node from the pool;
void matte_pool_recycle(mattePool_t * m, uint32_t id);

// Gets the instance at the ID as a raw pointer
void * matte_pool_fetch_raw(mattePool_t * m, uint32_t id);

#define matte_pool_fetch(__M__, __TYPE__, __ID__) (__TYPE__*)matte_pool_fetch_raw(__M__, __ID__);

#endif
