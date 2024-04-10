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


#ifndef H_MATTE__AU32__INCLUDED
#define H_MATTE__AU32__INCLUDED


#include <stdint.h>





/// Dynamically resizing container for uint32_t
///
///
typedef struct matteAU32_t matteAU32_t;
struct matteAU32_t {
    uint32_t allocSize;
    uint32_t size;
    uint32_t * data;
};


/// Returns a new, empty array 
///
/// sizeofType refers to the size of the elements that the array will hold 
/// the most convenient way to do this is to use "sizeof()"
matteAU32_t * matte_au32_create();

/// Destroys the container and buffer that it manages.
///
void matte_au32_destroy(
    /// The array to destroy.
    matteAU32_t * array
);

void matte_au32_set_size(
    matteAU32_t * array,
    uint32_t size 
);

#define matte_au32_push(__T__, __VAL__) \
    matte_au32_set_size(__T__, (__T__)->size+1);\
    (__T__)->data[(__T__)->size-1] = __VAL__;


#define matte_au32_remove(__T__, __IND__) \
    (__T__)->data[__IND__] = (__T__)->data[(__T__)->size-1];\
    (__T__)->size-=1;









#endif
