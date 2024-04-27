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

#include "matte_table.h"
#include "matte_string.h"
#include "matte_array.h"
#include "matte.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#ifdef MATTE_DEBUG
#include <assert.h>
#endif


#define TRUE 1
#define FALSE 0

#define table_bucket_start_size 8      
#define table_bucket_fill_rate 0.80       //<- larger than this triggers resize
#define table_bucket_fill_amount 16

// holds an individual key-value pair
typedef struct matteTableEntry_t matteTableEntry_t;
struct matteTableEntry_t {
    int keyLen;
    void * value;
    void * key;
};


typedef struct matteTableBucket_t matteTableBucket_t;
struct matteTableBucket_t {
    matteTableEntry_t * entries;
    uint32_t size;
    uint32_t alloc;
};



// converts data to a hash
typedef uint32_t (*KeyHashFunction)(const void * data, uint32_t param);

// compares data 
typedef int (*KeyCompareFunction)(const void * dataA, const void * dataB, uint32_t param);


typedef void (*KeyCleanFunction)(void * dataA);


struct matteTable_t {
    // numer of keys
    uint32_t size;

    // number of buckets
    uint32_t nBuckets;

    // actual buckets
    matteTableBucket_t * buckets;
    
    uint32_t bucketsFilled;



    // Converts key data to a hash
    KeyHashFunction hash;

    // Compares keys
    KeyCompareFunction keyCmp;

    // frees a key once its destroyed
    KeyCleanFunction keyRemove;

    
    // for static key sizes. If -1, is dynamic (i.e. strings)
    // if 0, the key has no allocated size (pointer / value direct keys)
    int keyLen;



    // TODO: instead of freeing them, just put them back into a reserve 
    // stack (thread-local reserve pool?)

};




struct matteTableIter_t {
    // source table
    matteTable_t * src;

    // current reference entry
    matteTableEntry_t * current;

    // current bucket in reference
    uint32_t currentBucketID;
    
    // current bucket index;
    uint32_t currentBucketIndex;

    // whether the iterator has reached the end.
    int isEnd;
};

static void key_destroy_dont(void * k) {}

// convert a table to a bucket index
static uint32_t hash_to_index(const matteTable_t * t, uint32_t hash) {
    return (hash*11) % (t->nBuckets);
}


// djb
static uint32_t hash_fn_buffer(uint8_t * data, uint32_t len) {
    uint32_t hash = 5381;

    uint32_t i;
    for(i = 0; i < len; ++i, ++data) {
        hash = (hash<<5) + hash + *data;
    } 
    return hash;
}

static int key_cmp_fn_buffer(const void * a, const void * b, uint32_t len) {
    return memcmp(a, b, len)==0;
}


static int key_cmp_fn_c_str(const void * a, const void * b, uint32_t len) {
    return strcmp((char*)a, (char*)b)==0;
}


static uint32_t hash_fn_matte_str(uint8_t * src, uint32_t len) {
    matteString_t * str = (matteString_t*)src;
    return matte_string_get_hash(str);
}

static int key_cmp_fn_matte_str(const void * a, const void * b, uint32_t len) {
    return matte_string_test_eq((matteString_t*)a, (matteString_t*)b);
}

static void bucket_add(matteTableBucket_t * bucket, matteTableEntry_t entry) {
    // for now...
    if (bucket->size + 1 > bucket->alloc) {
        uint32_t oldSize = bucket->alloc;
        matteTableEntry_t * oldEntries = bucket->entries;
        
        if (bucket->alloc == 0) bucket->alloc = 1;
        else                    bucket->alloc *= 2;
        
        bucket->entries = (matteTableEntry_t*)matte_allocate(bucket->alloc*sizeof(matteTableEntry_t));
        uint32_t i;
        for(i = 0; i < oldSize; ++i)
            bucket->entries[i] = oldEntries[i];
        matte_deallocate(oldEntries);
    }
    
    
    bucket->entries[bucket->size++] = entry;
}

static void bucket_remove(matteTableBucket_t * bucket, uint32_t i) {
    bucket->entries[i] = bucket->entries[bucket->size-1];
    bucket->size--;
}




// pointer / value to a table directly
static uint32_t hash_fn_value(const void * data, uint32_t nu) {
    uint32_t hash = 5381;
    uint8_t * datab = (uint8_t*)&data;
    uint32_t i;
    for(i = 0; i < sizeof(void*); ++i, ++datab) {
        hash = (hash<<5) + hash + *datab;
    } 
    return hash;
    //return (uint32_t)(int64_t)data;
}

static int key_cmp_fn_value(const void * a, const void * b, uint32_t nu) {
    return a==b;
}








// resizes and redistributes all key-value pairs
static void matte_table_resize(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    matteTableBucket_t * buckets;
    matteTableEntry_t * entry;
    matteTableEntry_t * next;
    matteTableEntry_t * prev;

    uint32_t nEntries = 0, i, index;
    matteTableIter_t iter;


    // first, gather ALL key-value pairs
    buckets = t->buckets;
    nEntries = t->nBuckets;
    

    // then resize
    t->nBuckets *= 2;
    t->buckets = (matteTableBucket_t*)matte_allocate(t->nBuckets * sizeof(matteTableBucket_t));
    t->bucketsFilled = 0;

    // redistribute in-place
    uint32_t n;
    matteTableBucket_t * bucket;
    for(i = 0; i < nEntries; ++i) {
        bucket = buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            entry = bucket->entries+n;
            uint32_t keyLen = t->keyLen == -1 ? strlen((char*)entry->key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
            uint32_t hash = t->hash(entry->key, keyLen);
            index = hash_to_index(t, hash);

            matteTableBucket_t * next = t->buckets+index;
            if (next->size == table_bucket_fill_amount)
                t->bucketsFilled ++;
            bucket_add(next, *entry);
        }
        matte_deallocate(bucket->entries);
    }
    matte_deallocate(buckets);
}



int matte_table_get_size(const matteTable_t * table) {
    #ifdef MATTE_DEBUG
        assert(table && "matteTable_t pointer cannot be NULL.");
    #endif
    return table->size;
}   


static matteTable_t * matte_table_initialize(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif

    t->buckets = (matteTableBucket_t*)matte_allocate(sizeof(matteTableBucket_t) * table_bucket_start_size);
    t->nBuckets = table_bucket_start_size;

    t->size = 0;
    return t;
}



static matteTableEntry_t matte_table_new_entry(matteTable_t * t, const void * key, void * value, uint32_t keyLen, uint32_t hash) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif

    matteTableEntry_t out = {};

    out.value = value;
    out.keyLen = keyLen;
    // if the key is dynamically allocated, we need a local copy 
    if (keyLen) {    
        out.key = matte_allocate(keyLen);
        memcpy(out.key, key, keyLen);
    } else { // else simple copy (no modify)
        out.key = (void*)key;
    }
    return out;
}




matteTable_t * matte_table_create_hash_pointer() {
    matteTable_t * t = (matteTable_t*)matte_allocate(sizeof(matteTable_t));
    t->hash      = hash_fn_value;
    t->keyCmp    = key_cmp_fn_value;
    t->keyRemove = key_destroy_dont;
    t->keyLen    = 0;
    return matte_table_initialize(t);
}

matteTable_t * matte_table_create_hash_c_string() {
    matteTable_t * t = (matteTable_t*)matte_allocate(sizeof(matteTable_t));
    t->hash      = (KeyHashFunction)hash_fn_buffer;
    t->keyCmp    = key_cmp_fn_c_str;
    t->keyRemove = (KeyCleanFunction)matte_deallocate;
    t->keyLen    = -1;
    return matte_table_initialize(t);
}


matteTable_t * matte_table_create_hash_matte_string() {
    matteTable_t * t = (matteTable_t*)matte_allocate(sizeof(matteTable_t));
    t->hash      = (KeyHashFunction)hash_fn_matte_str;
    t->keyCmp    = key_cmp_fn_matte_str;
    t->keyRemove = (KeyCleanFunction)matte_string_destroy;
    t->keyLen    = -2;
    return matte_table_initialize(t);
}

matteTable_t * matte_table_create_hash_buffer(int size) {
    #ifdef MATTE_DEBUG
        assert(size > 0 && "matte_table_create_hash_buffer() requires non-zero size.");
    #endif
    matteTable_t * t = (matteTable_t*)matte_allocate(sizeof(matteTable_t));
    t->hash      = (KeyHashFunction)hash_fn_buffer;
    t->keyCmp    = key_cmp_fn_buffer;
    t->keyRemove = (KeyCleanFunction)matte_deallocate;
    t->keyLen    = size;
    return matte_table_initialize(t);    
}


void matte_table_destroy(matteTable_t * t) {
    matte_table_clear(t);
    matte_deallocate(t->buckets);
    matte_deallocate(t);
}


void matte_table_insert(matteTable_t * t, const void * key, void * value) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen((char*)key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableBucket_t * src  = t->buckets+bucketID;
    matteTableEntry_t * prev = NULL;
    int bucketLen = src->size;
    uint32_t i;
    // look for preexisting entry
    for(i = 0; i < bucketLen; ++i) {
        matteTableEntry_t * next = src->entries+i;
        if (next->keyLen == keyLen) { // hash must equal before key does, so 
                                 // this is an easy check
            if (t->keyCmp(key, next->key, next->keyLen)) {
                // update data for key
                next->value = value;
                return;            
            }
        }
    }    

    // case for 
    if (t->keyLen == -2) {
        key = matte_string_clone((matteString_t*)key);
    }    

    // add to chain at the end
    matteTableEntry_t entry = matte_table_new_entry(
        t, 

        key, 
        value,

        keyLen,
        hash
    );


    if (src->size == table_bucket_fill_amount)
        t->bucketsFilled ++;
    bucket_add(src, entry);
    t->size++;

    // invariant broken, expand and reform the hashtable.
    if (t->bucketsFilled / (float)t->nBuckets  > table_bucket_fill_rate) {
        matte_table_resize(t);
    }

}



void * matte_table_find(const matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen((char*)key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableBucket_t * bucket  = t->buckets+bucketID;
    matteTableEntry_t * next;
    // look for preexisting entry
    uint32_t i;
    for(i = 0; i < bucket->size; ++i) {
        next = bucket->entries+i;
        if (next->keyLen == keyLen) { // hash must equal before key does, so 
                                 // this is an easy check
            if (t->keyCmp(key, next->key, next->keyLen)) {
                return next->value;
            }
        }
    }    
    return NULL;
}

void * matte_table_get_any(
    const matteTable_t * table
) {
    uint32_t i;
    for(i = 0; i < table->nBuckets; ++i) {
        if (table->buckets[i].size)
            return table->buckets[i].entries[0].value;
    }
    return NULL;
}


int matte_table_entry_exists(const matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen((char*)key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableBucket_t * bucket  = t->buckets+bucketID;
    matteTableEntry_t * next;
    // look for preexisting entry
    uint32_t i;
    for(i = 0; i < bucket->size; ++i) {
        next = bucket->entries+i;
        if (next->keyLen == keyLen) { // hash must equal before key does, so 
            if (t->keyCmp(key, next->key, next->keyLen)) {
                return TRUE;
            }
        }
    }    
    return FALSE;
}


void matte_table_remove(matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen((char*)key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableBucket_t * bucket  = t->buckets+bucketID;
    matteTableEntry_t * next;
    // look for preexisting entry
    uint32_t i;
    for(i = 0; i < bucket->size; ++i) {
        next = bucket->entries+i;
        if (next->keyLen == keyLen) { // hash must equal before key does, so 
            if (t->keyCmp(key, next->key, next->keyLen)) {
                t->keyRemove(next->key);
                t->size--;
                bucket_remove(bucket, i);
                return;
            }
        }
    }    
}

int matte_table_is_empty(const matteTable_t * t) {
    return t->size == 0;
}

void matte_table_clear(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t i = 0;
    matteTableBucket_t * src;
    matteTableEntry_t * toRemove;
    uint32_t n;
    for(; i < t->nBuckets; ++i) {
        src = t->buckets+i;
        for(n = 0; n < src->size; ++n) {
            toRemove = src->entries+n;
            t->keyRemove(toRemove->key);
        }
        matte_deallocate(src->entries);
        src->entries = NULL;
        src->size = 0;
        src->alloc = 0;
    }
}

void matte_table_get_all_keys(const matteTable_t * t, matteArray_t * arr) {
    if (t->size == 0) return;

    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteTableBucket_t * bucket;
    matteTableEntry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            matte_array_push(arr, next->key);
        }
    }
}


void matte_table_get_limited_keys(const matteTable_t * t, matteArray_t * arr, int count) {
    if (t->size == 0) return;

    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteTableBucket_t * bucket;
    matteTableEntry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            matte_array_push(arr, next->key);
            if (arr->size >= count) return;
        }
    }
}



void matte_table_get_all_values(const matteTable_t * t, matteArray_t * arr) {
    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteTableBucket_t * bucket;
    matteTableEntry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            matte_array_push(arr, next->value);
        }
    }
}



matteTableIter_t * matte_table_iter_create() {
    return (matteTableIter_t*)matte_allocate(sizeof(matteTableIter_t));
}

void matte_table_iter_destroy(matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    matte_deallocate(t);
}




void matte_table_iter_start(matteTableIter_t * t, matteTable_t * src) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
        assert(src && "matteTable_t pointer cannot be NULL.");
    #endif
    t->src = src;
    t->currentBucketID = 0;
    t->currentBucketIndex = 0;
    t->current = NULL;
    t->isEnd = 0;
    
    
    matte_table_iter_proceed(t);
}

void matte_table_iter_proceed(matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    if (t->isEnd) return;

    matteTableBucket_t * buckets = t->src->buckets;

    matteTableBucket_t * bucket = buckets+t->currentBucketID;
    while(t->currentBucketIndex == bucket->size) {
        t->currentBucketIndex = 0;
        t->currentBucketID++;
        if (t->currentBucketID == t->src->nBuckets) {
            t->isEnd = 1;
            t->current = NULL;
            return;
        }
        bucket = buckets+(t->currentBucketID);
    }
    
    t->current = bucket->entries + t->currentBucketIndex++;
}

int matte_table_iter_is_end(const matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    return t->isEnd;
}

const void * matte_table_iter_get_key(const matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    if (t->current) {
        return t->current->key;
    }
    return NULL;
}

void * matte_table_iter_get_value(const matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    if (t->current) {
        return t->current->value;
    }
    return NULL;
}










