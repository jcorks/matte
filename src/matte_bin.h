/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the matte project (https://github.com/jcorks/matte)
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


#ifndef H_MATTE__BIN__INCLUDED
#define H_MATTE__BIN__INCLUDED


#include <stdint.h>
typedef struct matteArray_t matteArray_t;

/// Tags a pointer with a unique value ID. Mostly useful for internal 
/// implementations of ID systems where validation is needed.
typedef struct matteBin_t matteBin_t;

/// Creates a new bin to add objects to.
///
matteBin_t * matte_bin_create(void*(*)(), void (*)(void *));

/// Destroys the bin.
///
void matte_bin_destroy(
    /// The bin to destroy.
    matteBin_t * bin
);

/// Adds an object to the bin, its ID tag is returned.
/// The object must not be NULL.
///
void * matte_bin_add(
    /// The bin to add to.
    matteBin_t * bin,
    
    uint32_t *
);

/// Returns whether the bin contains the given value.
///
int matte_bin_contains(
    /// The bin to query.
    const matteBin_t * bin, 

    /// The ID to query.
    uint32_t id
);

/// Returns the object matched with the ID tag. If none exists
/// NULL is returned.
///
void * matte_bin_fetch(
    /// The bin to query.
    const matteBin_t * bin, 

    /// The object ID to query.
    uint32_t id
);

/// Removes an object from the bin.
///
void matte_bin_recycle(
    /// The bin to remove objects from.
    matteBin_t * bin, 

    /// The object ID to remove.
    uint32_t id
);





#endif
