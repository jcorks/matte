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


#ifndef H_MATTE__MVT2__INCLUDED
#define H_MATTE__MVT2__INCLUDED

#include "matte_store.h"


typedef struct matteArray_t matteArray_t;


/// HashMVT2 able to handle various kinds of keys.
/// For buffer and string keys, key copies are created, so 
/// the source key does not need to be kept in memory
/// once created.
///
typedef struct matteMVT2_t matteMVT2_t;




/// Creates a new MVT2 whose keys are a pointer value.
///
matteMVT2_t * matte_mvt2_create();




/// Frees the given MVT2.
///
void matte_mvt2_destroy(
    /// The MVT2 to destroy.
    matteMVT2_t * MVT2
);


/// Inserts a new key-value pair into the MVT2.
/// If a key is already within the MVT2, the value 
/// corresponding to that key is updated with the new copy.
///
/// Notes regarding keys: when copied into the MVT2, a value copy 
/// is performed if this hash MVT2's keys are pointer values.
/// If a buffer or string, a new buffer is stored and kept until 
/// key-value removal.
///
matteValue_t * matte_mvt2_insert(
    /// The MVT2 to insert content into.
    matteMVT2_t * MVT2, 
    
    /// The key associated with the value.
    matteValue_t key,

    /// The value to store.
    matteValue_t value
);



/// Returns the value corresponding to the given key.
/// If none is found, NULL is returned. Note that this 
/// implies useful output only if key-value pair contains 
/// non-null data. You can use "matte_mvt2_entry_exists()" to 
/// handle NULL values.
///
matteValue_t * matte_mvt2_find(
    /// The MVT2 to search.
    const matteMVT2_t * MVT2, 

    /// The key to search for.
    matteValue_t key
);



void matte_mvt2_get_all_keys(
    const matteMVT2_t * MVT2,
    matteArray_t * output
);


void matte_mvt2_get_limited_keys(
    const matteMVT2_t * MVT2,
    matteArray_t * output,
    int
);

void matte_mvt2_get_all_values(
    const matteMVT2_t * MVT2,
    matteArray_t * output
);


int matte_mvt2_get_size(const matteMVT2_t * MVT2);



/// Removes the key-value pair from the MVT2 whose key matches 
/// the one given. If no such pair exists, no action is taken.
///
void matte_mvt2_remove(
    /// The MVT2 to remove content from.
    matteMVT2_t * MVT2, 

    /// The key referring to the element to remove.
    matteValue_t key
);


/// Returns whether the MVT2 has entries.
///
int matte_mvt2_is_empty(
    /// The MVT2 to query.
    const matteMVT2_t * MVT2
);

/// Removes all key-value pairs.
///
void matte_mvt2_clear(
    /// The MVT2 to clear.
    matteMVT2_t * MVT2
);



#endif
