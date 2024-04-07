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

#include "matte_mvt2.h"
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

#define MVT2_bucket_start_size 4      
#define MVT2_bucket_fill_rate 0.80       //<- larger than this triggers resize
#define MVT2_bucket_fill_amount 4

// holds an individual key-value pair
typedef struct matteMVT2Entry_t matteMVT2Entry_t;
struct matteMVT2Entry_t {
    uint32_t indexPacked;
    uint32_t key;
    uint32_t binID;
};


typedef struct matteMVT2Bucket_t matteMVT2Bucket_t;
struct matteMVT2Bucket_t {
    matteMVT2Entry_t * entries;
    uint32_t size;
    uint32_t alloc;
};


static matteArray_t * mainReserve_dead = NULL;
static matteArray_t * mainReserve = NULL;

static matteArray_t * mainReserve_ID = NULL;
static matteArray_t * mainReserve_ID_dead = NULL;



struct matteMVT2_t {
    // numer of keys
    uint32_t size;

    // number of buckets
    uint32_t nBuckets;

    // actual buckets
    matteMVT2Bucket_t * buckets;
    
    uint32_t bucketsFilled;
};


static void bucket_add(matteMVT2Bucket_t * bucket, matteMVT2Entry_t entry) {
    // for now...
    if (bucket->size + 1 > bucket->alloc) {
        uint32_t oldSize = bucket->alloc;
        matteMVT2Entry_t * oldEntries = bucket->entries;
        
        if (bucket->alloc == 0) bucket->alloc  = 1;
        else                    bucket->alloc += MVT2_bucket_fill_amount/2;
        
        bucket->entries = matte_allocate(bucket->alloc*sizeof(matteMVT2Entry_t));
        uint32_t i;
        for(i = 0; i < oldSize; ++i)
            bucket->entries[i] = oldEntries[i];
        matte_deallocate(oldEntries);
    }
    
    matteMVT2Entry_t * entryNew = &bucket->entries[bucket->size++];
    *entryNew = entry;
}

static void bucket_remove(matteMVT2Bucket_t * bucket, uint32_t i) {
    bucket->entries[i] = bucket->entries[bucket->size-1];
    bucket->size--;
}








// resizes and redistributes all key-value pairs
static void matte_mvt2_resize(matteMVT2_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteMVT2_t pointer cannot be NULL.");
    #endif
    matteMVT2Bucket_t * buckets;
    matteMVT2Entry_t * entry;
    matteMVT2Entry_t * next;
    matteMVT2Entry_t * prev;

    uint32_t nEntries = 0, i, index;


    // first, gather ALL key-value pairs
    buckets = t->buckets;
    nEntries = t->nBuckets;
    

    // then resize
    t->nBuckets = 3 + (t->nBuckets * 1.4);
    t->buckets = (matteMVT2Bucket_t*)matte_allocate(t->nBuckets * sizeof(matteMVT2Bucket_t));
    t->bucketsFilled = 0;

    // redistribute in-place
    uint32_t n;
    matteMVT2Bucket_t * bucket;
    for(i = 0; i < nEntries; ++i) {
        bucket = buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            entry = bucket->entries+n;
            matteMVT2Bucket_t * next = t->buckets+(entry->key%t->nBuckets);
            if (next->size == MVT2_bucket_fill_amount)
                t->bucketsFilled ++;
            bucket_add(next, *entry);
        }
        matte_deallocate(bucket->entries);
    }
    matte_deallocate(buckets);
}



int matte_mvt2_get_size(const matteMVT2_t * MVT2) {
    #ifdef MATTE_DEBUG
        assert(MVT2 && "matteMVT2_t pointer cannot be NULL.");
    #endif
    return MVT2->size;
}   

#define PACKED_TYPE(__IP__)  ((__IP__) & 7)
#define PACKED_INDEX(__IP__) ((__IP__) >> 3)

static void remove_reserve(uint32_t indexPacked) {
    uint32_t index;
    switch(PACKED_TYPE(indexPacked)) {
      case 0:
      case 1:
        return;
        
      case 2:
        index = PACKED_INDEX(indexPacked);
        matte_array_push(mainReserve_dead, index);
        return;
      case 3:
        index = PACKED_INDEX(indexPacked);
        matte_array_push(mainReserve_ID_dead, index);
        return;        
    }
}

static matteValue_t valEmpty = {};
static matteValue_t valTrue  = {};
static matteValue_t valFalse = {};
static matteValue_t valNumber = {};
static matteValue_t valOther = {};


static matteValue_t * get_reserve(uint32_t indexPacked) {
    switch(PACKED_TYPE(indexPacked)) {
      case 0: 
        return &valEmpty;
      case 1: 
        if (PACKED_INDEX(indexPacked) & 1)
            return &valTrue;
        else 
            return &valFalse; 
      case 2:
        
        valNumber.value.number = matte_array_at(mainReserve, double, PACKED_INDEX(indexPacked));
        return &valNumber;
        
      default:
        valOther.binID = PACKED_TYPE(indexPacked);
        valOther.value.id = matte_array_at(mainReserve_ID, uint32_t, PACKED_INDEX(indexPacked));
        return &valOther;        
    }
}

static uint32_t add_reserve(matteValue_t * v) {
    uint32_t ind;
    uint32_t indReal;
    matteArray_t * reserve;
    matteArray_t * reserve_dead;
    switch(v->binID) {
      case 0: return 0;
      case 1: 
        if (v->value.boolean)
            return 9; //  1001;
        else 
            return 1; //  0001; 
      case 2:

        reserve      = mainReserve;
        reserve_dead = mainReserve_dead;

        if (matte_array_get_size(reserve_dead)) {
            ind = matte_array_at(reserve_dead, uint32_t, reserve_dead->size-1);
            reserve_dead->size -= 1;
            matte_array_at(reserve, double, ind) = v->value.number;
        } else {
            ind = matte_array_get_size(reserve);
            matte_array_push(reserve, v->value.number);
        }
        return (ind << 3) + v->binID;
      
      default:
        reserve      = mainReserve_ID;
        reserve_dead = mainReserve_ID_dead;

        if (matte_array_get_size(reserve_dead)) {
            ind = matte_array_at(reserve_dead, uint32_t, reserve_dead->size-1);
            reserve_dead->size -= 1;
            matte_array_at(reserve, uint32_t, ind) = v->value.id;
        } else {
            ind = matte_array_get_size(reserve);
            matte_array_push(reserve, v->value.id);
        }
        return (ind << 3) + v->binID;
    }

}


matteMVT2_t * matte_mvt2_create() {
    matteMVT2_t * t = matte_allocate(sizeof(matteMVT2_t));    
    if (mainReserve == NULL) {
        mainReserve = matte_array_create(sizeof(double));
        mainReserve_dead = matte_array_create(sizeof(uint32_t));

        mainReserve_ID = matte_array_create(sizeof(uint32_t));
        mainReserve_ID_dead = matte_array_create(sizeof(uint32_t));

        valTrue.binID = 1;
        valTrue.value.boolean = 1;

        valFalse.binID = 1;
        valFalse.value.boolean = 0;

        valNumber.binID = 2;
    }

    t->buckets = (matteMVT2Bucket_t*)matte_allocate(sizeof(matteMVT2Bucket_t) * MVT2_bucket_start_size);
    t->nBuckets = MVT2_bucket_start_size;
    t->size = 0;
    return t;
}







void matte_mvt2_destroy(matteMVT2_t * t) {
    matte_mvt2_clear(t);
    matte_deallocate(t->buckets);
    matte_deallocate(t);
}


matteValue_t * matte_mvt2_insert(matteMVT2_t * t, matteValue_t key, matteValue_t v) {
    #ifdef MATTE_DEBUG
        assert(t && "matteMVT2_t pointer cannot be NULL.");
    #endif
        
    // invariant broken, expand and reform the hashMVT2.
    if (t->bucketsFilled / (float)t->nBuckets  > MVT2_bucket_fill_rate) {
        matte_mvt2_resize(t);
    }

    matteMVT2Bucket_t * src  = t->buckets+(key.value.id%t->nBuckets);
    int bucketLen = src->size;
    uint32_t i;
    // look for preexisting entry
    for(i = 0; i < bucketLen; ++i) {
        matteMVT2Entry_t * next = src->entries+i;
        if (next->key == key.value.id && next->binID == key.binID) {
            remove_reserve(next->indexPacked);
            next->indexPacked = add_reserve(&v);
            return get_reserve(next->indexPacked);
        }
    }    

    // add to chain at the end
    matteMVT2Entry_t entry = {};
    entry.key = key.value.id;
    entry.binID = key.binID;
    entry.indexPacked = add_reserve(&v);


    if (src->size == MVT2_bucket_fill_amount)
        t->bucketsFilled ++;

    t->size++;
    bucket_add(src, entry);
    return get_reserve(entry.indexPacked);
}



matteValue_t * matte_mvt2_find(const matteMVT2_t * t, matteValue_t key) {
    #ifdef MATTE_DEBUG
        assert(t && "matteMVT2_t pointer cannot be NULL.");
    #endif

    matteMVT2Bucket_t * bucket  = t->buckets+(key.value.id%t->nBuckets);
    matteMVT2Entry_t * next;
    // look for preexisting entry
    uint32_t i;
    for(i = 0; i < bucket->size; ++i) {
        next = bucket->entries+i;
        if (next->key == key.value.id && next->binID == key.binID) {
            return get_reserve(next->indexPacked);
        }
    }    
    return NULL;
}




void matte_mvt2_remove(matteMVT2_t * t, matteValue_t key) {
    matteMVT2Bucket_t * bucket  = t->buckets+(key.value.id%t->nBuckets);
    matteMVT2Entry_t * next;
    // look for preexisting entry
    uint32_t i;
    for(i = 0; i < bucket->size; ++i) {
        next = bucket->entries+i;
        if (next->key == key.value.id && next->binID == key.binID) {
            remove_reserve(next->indexPacked);
            bucket_remove(bucket, i);
            t->size--;
            return;
        }
    }    
}

int matte_mvt2_is_empty(const matteMVT2_t * t) {
    return t->size == 0;
}

void matte_mvt2_clear(matteMVT2_t * t) {
    #ifdef MATTE_DEBUG
        assert(t && "matteMVT2_t pointer cannot be NULL.");
    #endif
    uint32_t i = 0;
    matteMVT2Bucket_t * src;
    matteMVT2Entry_t * toRemove;
    uint32_t n;
    for(; i < t->nBuckets; ++i) {
        src = t->buckets+i;
        for(n = 0; n < src->size; ++n) {
            remove_reserve(src->entries[n].indexPacked);
        }   
        matte_deallocate(src->entries);
        src->entries = NULL;
        src->size = 0;
        src->alloc = 0;
    }
}

void matte_mvt2_get_all_keys(const matteMVT2_t * t, matteArray_t * arr) {
    if (t->size == 0) return;

    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteMVT2Bucket_t * bucket;
    matteMVT2Entry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            
            matteValue_t key = {};
            key.binID = next->binID;
            key.value.id = next->key;
            matte_array_push(arr, key);
        }
    }
}


void matte_mvt2_get_limited_keys(const matteMVT2_t * t, matteArray_t * arr, int count) {
    if (t->size == 0) return;

    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteMVT2Bucket_t * bucket;
    matteMVT2Entry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            matte_array_push(arr, next->key);
            if (arr->size >= count) return;
        }
    }
}



void matte_mvt2_get_all_values(const matteMVT2_t * t, matteArray_t * arr) {
    // look for preexisting entry
    uint32_t i;
    uint32_t n;
    
    matteMVT2Bucket_t * bucket;
    matteMVT2Entry_t * next;
    for(i = 0; i < t->nBuckets; ++i) {
        bucket = t->buckets+i;
        for(n = 0; n < bucket->size; ++n) {
            next = bucket->entries+n;
            matteValue_t v = *get_reserve(next->indexPacked);
            matte_array_push(arr, v);
        }
    }
}











