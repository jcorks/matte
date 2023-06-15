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


#ifndef H_MATTE__TABLE__INCLUDED
#define H_MATTE__TABLE__INCLUDED

/// Hashtable able to handle various kinds of keys.
/// For buffer and string keys, key copies are created, so 
/// the source key does not need to be kept in memory
/// once created.
///
typedef struct matteTable_t matteTable_t;

/// Creates a new table whose keys are C-strings.
///
matteTable_t * matte_table_create_hash_c_string();

/// Creates a new table while keys are matte strings.
///
matteTable_t * matte_table_create_hash_matte_string();


/// Creates a new table whose keys are a byte-buffer of 
/// the specified length.
///
matteTable_t * matte_table_create_hash_buffer(
    /// The the size of keys in bytes.
    int keySize
);


/// Creates a new table whose keys are a pointer value.
///
matteTable_t * matte_table_create_hash_pointer();




/// Frees the given table.
///
void matte_table_destroy(
    /// The table to destroy.
    matteTable_t * table
);


/// Inserts a new key-value pair into the table.
/// If a key is already within the table, the value 
/// corresponding to that key is updated with the new copy.
///
/// Notes regarding keys: when copied into the table, a value copy 
/// is performed if this hash table's keys are pointer values.
/// If a buffer or string, a new buffer is stored and kept until 
/// key-value removal.
///
void matte_table_insert(
    /// The table to insert content into.
    matteTable_t * table, 
    
    /// The key associated with the value.
    const void * key, 

    /// The value to store.
    void * value
);

/// Same as matte_table_insert, but treats the key as a signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_insert_by_int(__T__,__K__,__V__) (matte_table_insert(__T__, (void*)(intptr_t)__K__, __V__))

/// Same as matte_table_insert, but treats the key as an un signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_insert_by_uint(__T__,__K__,__V__) (matte_table_insert(__T__, (void*)(uintptr_t)__K__, __V__))


/// Returns the value corresponding to the given key.
/// If none is found, NULL is returned. Note that this 
/// implies useful output only if key-value pair contains 
/// non-null data. You can use "matte_table_entry_exists()" to 
/// handle NULL values.
///
void * matte_table_find(
    /// The table to search.
    const matteTable_t * table, 

    /// The key to search for.
    const void * key
);


/// Returns a single value within the table.
/// If none is found, NULL is returned.
///
void * matte_table_get_any(
    /// The table to query
    const matteTable_t * table
);

/// Same as matte_table_find, but treats the key as a signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_find_by_int(__T__,__K__) (matte_table_find(__T__, (void*)(intptr_t)(__K__)))

/// Same as matte_table_find, but treats the key as an unsignedinteger
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_find_by_uint(__T__,__K__) (matte_table_find(__T__, (void*)(uintptr_t)(__K__)))

/// Returns TRUE if an entry correspodning to the 
/// given key exists and FALSE otherwise.
///
int matte_table_entry_exists(
    /// The table to query.
    const matteTable_t * table, 

    /// The key to search for.
    const void * key
);


/// Removes the key-value pair from the table whose key matches 
/// the one given. If no such pair exists, no action is taken.
///
void matte_table_remove(
    /// The table to remove content from.
    matteTable_t * table, 

    /// The key referring to the element to remove.
    const void * key
);

/// Same as matte_table_remove, but treats the key as a signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_remove_by_int(__T__,__K__) (matte_table_remove(__T__, (void*)(intptr_t)(__K__)))

/// Same as matte_table_remove, but treats the key as an unsignedinteger
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define matte_table_remove_by_uint(__T__,__K__) (matte_table_remove(__T__, (void*)(uintptr_t)(__K__)))


/// Returns whether the table has entries.
///
int matte_table_is_empty(
    /// The table to query.
    const matteTable_t * table
);

/// Removes all key-value pairs.
///
void matte_table_clear(
    /// The table to clear.
    matteTable_t * table
);






/// Helper class for iterating through hash tables
typedef struct matteTableIter_t matteTableIter_t;


/// Creates a new hash table iterator.
/// This iterator can be used with any table, but needs 
/// to be "started" with the table in question.
///
matteTableIter_t * matte_table_iter_create();


/// Destroys a table iter.
///
void matte_table_iter_destroy(
    /// The iterator to destroy.
    matteTableIter_t * iter
);

/// Begins the iterating process by initializing the iter 
/// to contain the first key-value pair within the table.
///
void matte_table_iter_start(
    /// The iterator to initialize.
    matteTableIter_t * iter, 

    /// The table for the interator to refer to.
    matteTable_t * table
);


/// Goes to the next available key-value pair in the table 
/// 
void matte_table_iter_proceed(
    /// The iterator to modify.
    matteTableIter_t * iter
);

/// Returns whether the end of the table has been reached.
///
int matte_table_iter_is_end(
    /// The iterator to query.
    const matteTableIter_t * iter
);


/// Returns the key (owned by the table) for the current 
/// key-value pair. If none, returns NULL.
///
const void * matte_table_iter_get_key(
    /// The iterator to query.
    const matteTableIter_t * iter
);

#define matte_table_iter_get_key_uint(__T__) ((uint32_t)(uintptr_t)matte_table_iter_get_key(__T__))


/// Returns the value for the current 
/// key-value pair. If none, returns NULL.
///
void * matte_table_iter_get_value(
    /// The iterator to query.
    const matteTableIter_t * iter
);


#endif
