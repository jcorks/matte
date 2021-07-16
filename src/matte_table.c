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

#include <matte_table.h>
#include <matte_string.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#ifdef MATTE_DEBUG
#include <assert.h>
#endif




#define table_bucket_reserve_size 256
#define table_bucket_start_size 32      
#define table_bucket_max_size 32        //<- larger than this triggers resize


// holds an individual key-value pair
typedef struct matteTableEntry_t matteTableEntry_t;
struct matteTableEntry_t {
    matteTableEntry_t * next;
    uint32_t hash;
    int keyLen;
    void * value;
    void * key;
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
    matteTableEntry_t ** buckets;



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
    return strcmp(a, b)==0;
}


static uint32_t hash_fn_matte_str(uint8_t * src, uint32_t len) {
    matteString_t * str = (void*)src;
    uint8_t * data = matte_string_get_byte_data(str);
    len = matte_string_get_byte_length(str);

    uint32_t hash = 5381;

    uint32_t i;
    for(i = 0; i < len; ++i, ++data) {
        hash = (hash<<5) + hash + *data;
    } 
    return hash;
}

static int key_cmp_fn_matte_str(const void * a, const void * b, uint32_t len) {
    return matte_string_test_eq(a, b);
}






// pointer / value to a table directly
static uint32_t hash_fn_value(const void * data, uint32_t nu) {
    return (uint32_t)(int64_t)data;
}

static int key_cmp_fn_value(const void * a, const void * b, uint32_t nu) {
    return a==b;
}








// resizes and redistributes all key-value pairs
static void matte_table_resize(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    matteTableEntry_t ** entries = malloc(sizeof(matteTableEntry_t)*t->size);
    matteTableEntry_t * entry;
    matteTableEntry_t * next;
    matteTableEntry_t * prev;

    uint32_t nEntries = 0, i, index;
    matteTableIter_t iter;


    // first, gather ALL key-value pairs
    memset(&iter, 0, sizeof(matteTableIter_t));
    matte_table_iter_start(&iter, t);
    for(; !matte_table_iter_is_end(&iter); matte_table_iter_proceed(&iter)) {
        entries[nEntries++] = iter.current;
    }
    

    // then resize
    t->nBuckets *= 2;
    free(t->buckets);
    t->buckets = calloc(t->nBuckets, sizeof(matteTableEntry_t*));


    // redistribute in-place
    for(i = 0; i < nEntries; ++i) {
        entry = entries[i];
        entry->next = NULL;

        index = hash_to_index(t, entry->hash);


        next = t->buckets[index];
        prev = NULL;    
        while(next) {
            prev = next;
            next = next->next;
        }


        if (next == prev) {
            t->buckets[index] = entry;
        } else {
            prev->next = entry;
        }
    }
    free(entries);
        

    
}





static matteTable_t * matte_table_initialize(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif

    t->buckets = calloc(sizeof(matteTableEntry_t*), table_bucket_start_size);
    t->nBuckets = table_bucket_start_size;

    t->size = 0;
    return t;
}



static matteTableEntry_t * matte_table_new_entry(matteTable_t * t, const void * key, void * value, uint32_t keyLen, uint32_t hash) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif

    matteTableEntry_t * out;

    out = malloc(sizeof(matteTableEntry_t));
    out->value = value;
    out->next = NULL;
    out->keyLen = keyLen;
    // if the key is dynamically allocated, we need a local copy 
    if (keyLen) {    
        out->key = malloc(keyLen);
        memcpy(out->key, key, keyLen);
    } else { // else simple copy (no modify)
        out->key = (void*)key;
    }
    out->hash = hash;
    return out;
}


static void matte_table_remove_entry(matteTable_t * t, matteTableEntry_t * entry) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
        assert(entry && "matteTableEntry_t pointer cannot be NULL.");
    #endif

    t->keyRemove(entry->key);
    free(entry);
}


matteTable_t * matte_table_create_hash_pointer() {
    matteTable_t * t = calloc(sizeof(matteTable_t), 1);
    t->hash      = hash_fn_value;
    t->keyCmp    = key_cmp_fn_value;
    t->keyRemove = key_destroy_dont;
    t->keyLen    = 0;
    return matte_table_initialize(t);
}

matteTable_t * matte_table_create_hash_c_string() {
    matteTable_t * t = calloc(sizeof(matteTable_t), 1);
    t->hash      = (KeyHashFunction)hash_fn_buffer;
    t->keyCmp    = key_cmp_fn_c_str;
    t->keyRemove = (KeyCleanFunction)free;
    t->keyLen    = -1;
    return matte_table_initialize(t);
}


matteTable_t * matte_table_create_hash_matte_string() {
    matteTable_t * t = calloc(sizeof(matteTable_t), 1);
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
    matteTable_t * t = calloc(sizeof(matteTable_t), 1);
    t->hash      = (KeyHashFunction)hash_fn_buffer;
    t->keyCmp    = key_cmp_fn_buffer;
    t->keyRemove = (KeyCleanFunction)free;
    t->keyLen    = size;
    return matte_table_initialize(t);    
}


void matte_table_destroy(matteTable_t * t) {
    matte_table_clear(t);
    free(t->buckets);
    free(t);
}


void matte_table_insert(matteTable_t * t, const void * key, void * value) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen(key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableEntry_t * src  = t->buckets[bucketID];
    matteTableEntry_t * prev = NULL;
    int bucketLen = 0;

    // look for preexisting entry
    while(src) {

        if (src->hash   == hash && 
            src->keyLen == keyLen) { // hash must equal before key does, so 
                                 // this is an easy check
            if (t->keyCmp(key, src->key, src->keyLen)) {
                // update data for key
                src->value = value;
                return;            
            }
        }
        prev = src;
        src = src->next;
        bucketLen++;
    }    

    // case for 
    if (t->keyLen == -2) {
        key = matte_string_clone(key);
    }    

    // add to chain at the end
    src = matte_table_new_entry(
        t, 

        key, 
        value,

        keyLen,
        hash
    );


    if (prev) {
        prev->next = src;
    } else { // start of new chain
        t->buckets[bucketID] = src;
    }
    t->size++;

    // invariant broken, expand and reform the hashtable.
    if (bucketLen > table_bucket_max_size) {
        matte_table_resize(t);

        // try again.
        matte_table_insert(t, key, value);
        return;
    }

}



void * matte_table_find(const matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen(key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableEntry_t * src  = t->buckets[bucketID];

    // look for preexisting entry
    while(src) {

        if (src->hash   == hash && 
            src->keyLen == keyLen) { // hash must equal before key does, so 
                                 // this is an easy check
            if (t->keyCmp(key, src->key, src->keyLen)) {

                return src->value;
            }
        }
        src = src->next;
    }    
    return NULL;
}

int matte_table_entry_exists(const matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen(key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableEntry_t * src  = t->buckets[bucketID];

    // look for preexisting entry
    while(src) {

        if (src->hash   == hash && 
            src->keyLen == keyLen) { // hash must equal before key does, so 
            if (t->keyCmp(key, src->key, src->keyLen)) {
                return TRUE;
            }
        }
        src = src->next;
    }    
    return FALSE;
}


void matte_table_remove(matteTable_t * t, const void * key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t keyLen = t->keyLen == -1 ? strlen(key)+1 : t->keyLen == -2 ? 0 : t->keyLen;
    uint32_t hash = t->hash(key, keyLen);
    uint32_t bucketID = hash_to_index(t, hash);


    matteTableEntry_t * src  = t->buckets[bucketID];
    matteTableEntry_t * prev = NULL;

    // look for preexisting entry
    while(src) {

        if (src->hash   == hash && 
            src->keyLen == keyLen) { // hash must equal before key does, so 
            if (t->keyCmp(key, src->key, src->keyLen)) {

                if (prev) {
                    // skip over entry
                    prev->next = src->next;
                } else {
                    t->buckets[bucketID] = src->next;                    
                }
                matte_table_remove_entry(t, src);
                t->size--;

                return;
            }
        }
        prev = src;
        src = src->next;
    }    
}

int matte_table_is_empty(const matteTable_t * t) {
    return t->size != 0;
}

void matte_table_clear(matteTable_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTable_t pointer cannot be NULL.");
    #endif
    uint32_t i = 0;
    matteTableEntry_t * src;
    matteTableEntry_t * toRemove;

    for(; i < t->nBuckets; ++i) {
        src = t->buckets[i];
        while(src) {
            toRemove = src;
            src = toRemove->next;

            matte_table_remove_entry(t, toRemove);
        }

        t->buckets[i] = NULL;
    }
}





matteTableIter_t * matte_table_iter_create() {
    return calloc(sizeof(matteTableIter_t), 1);
}

void matte_table_iter_destroy(matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    free(t);
}


void matte_table_iter_start(matteTableIter_t * t, matteTable_t * src) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
        assert(src && "matteTable_t pointer cannot be NULL.");
    #endif
    t->src = src;
    t->currentBucketID = 0;
    t->current = src->buckets[0];
    t->isEnd = 0;
    
    if (!t->current)
        matte_table_iter_proceed(t);  

}

void matte_table_iter_proceed(matteTableIter_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteTableIter_t pointer cannot be NULL.");
    #endif
    if (t->isEnd) return;
    
    if (t->current) {
        t->current = t->current->next;

        // iter points to next in bucket
        if (t->current)
            return;
    }

    
    // need to move to next buckt
    uint32_t i = t->currentBucketID+1;
    for(; i < t->src->nBuckets; ++i) {
        if (t->src->buckets[i]) {
            t->current = t->src->buckets[i];
            t->currentBucketID = i;
            return;
        }        
    }

    t->current = NULL;
    t->isEnd = TRUE;
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










