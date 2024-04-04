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


#ifndef H_MATTE__U32MVT__INCLUDED
#define H_MATTE__U32MVT__INCLUDED

#include "matte_store.h"


typedef struct matteArray_t matteArray_t;


/// HashU32MVT able to handle various kinds of keys.
/// For buffer and string keys, key copies are created, so 
/// the source key does not need to be kept in memory
/// once created.
///
typedef struct matteU32MVT_t matteU32MVT_t;




/// Creates a new U32MVT whose keys are a pointer value.
///
matteU32MVT_t * matte_u32mvt_create();




/// Frees the given U32MVT.
///
void matte_u32mvt_destroy(
    /// The U32MVT to destroy.
    matteU32MVT_t * U32MVT
);


/// Inserts a new key-value pair into the U32MVT.
/// If a key is already within the U32MVT, the value 
/// corresponding to that key is updated with the new copy.
///
/// Notes regarding keys: when copied into the U32MVT, a value copy 
/// is performed if this hash U32MVT's keys are pointer values.
/// If a buffer or string, a new buffer is stored and kept until 
/// key-value removal.
///
matteValue_t * matte_u32mvt_insert(
    /// The U32MVT to insert content into.
    matteU32MVT_t * U32MVT, 
    
    /// The key associated with the value.
    uint32_t key,

    /// The value to store.
    matteValue_t value
);



/// Returns the value corresponding to the given key.
/// If none is found, NULL is returned. Note that this 
/// implies useful output only if key-value pair contains 
/// non-null data. You can use "matte_u32mvt_entry_exists()" to 
/// handle NULL values.
///
matteValue_t * matte_u32mvt_find(
    /// The U32MVT to search.
    const matteU32MVT_t * U32MVT, 

    /// The key to search for.
    uint32_t key
);



void matte_u32mvt_get_all_keys(
    const matteU32MVT_t * U32MVT,
    matteArray_t * output
);


void matte_u32mvt_get_limited_keys(
    const matteU32MVT_t * U32MVT,
    matteArray_t * output,
    int
);

void matte_u32mvt_get_all_values(
    const matteU32MVT_t * U32MVT,
    matteArray_t * output
);


int matte_u32mvt_get_size(const matteU32MVT_t * U32MVT);



/// Removes the key-value pair from the U32MVT whose key matches 
/// the one given. If no such pair exists, no action is taken.
///
void matte_u32mvt_remove(
    /// The U32MVT to remove content from.
    matteU32MVT_t * U32MVT, 

    /// The key referring to the element to remove.
    uint32_t key
);


/// Returns whether the U32MVT has entries.
///
int matte_u32mvt_is_empty(
    /// The U32MVT to query.
    const matteU32MVT_t * U32MVT
);

/// Removes all key-value pairs.
///
void matte_u32mvt_clear(
    /// The U32MVT to clear.
    matteU32MVT_t * U32MVT
);



#endif
