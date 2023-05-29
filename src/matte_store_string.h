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
#ifndef H_MATTE__STRING_STORE__INCLUDED
#define H_MATTE__STRING_STORE__INCLUDED

#include <stdint.h>
typedef struct matteString_t matteString_t;
typedef struct matteStringStore_t matteStringStore_t;

matteStringStore_t * matte_string_store_create();

void matte_string_store_destroy(matteStringStore_t *);

uint32_t matte_string_store_ref(matteStringStore_t *, const matteString_t *);

// same as matte_string_store_internalize(), but accepts a c string for convenience
uint32_t matte_string_store_ref_cstring(matteStringStore_t *, const char *);

void matte_string_store_ref_id(matteStringStore_t *, uint32_t);


void matte_string_store_unref(matteStringStore_t *, uint32_t);

const matteString_t * matte_string_store_find(const matteStringStore_t *, uint32_t);


// null if fail
const matteString_t * matte_string_store_find(const matteStringStore_t *, uint32_t);

#ifdef MATTE_DEBUG
void matte_string_store_print(matteStringStore_t * h);
#endif


#endif
