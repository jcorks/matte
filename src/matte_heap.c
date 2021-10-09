#include "matte_heap.h"
#include "matte_bin.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_string.h"
#include "matte_bytecode_stub.h"
#include "matte_vm.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef MATTE_DEBUG 
    #include <assert.h>
#endif

typedef struct {
    // name of the type
    matteString_t * name;
    
    // All the typecodes that this type can validly be used as in an 
    // isa comparison
    matteArray_t * isa;
} MatteTypeData;

struct matteHeap_t {    
    // 1 -> number. Each pointer is just a double
    // 2 -> boolean 
    // 3 -> string 
    // 4 -> object
    matteBin_t * sortedHeaps[6];
    
    
    
    
    // objects with suspected ref count 0.
    // needs a double check before confirming.
    // matteObject_t *
    matteTable_t * toSweep;


    // Collection of objects that were detected to have been 
    // untraceable to a root object. This means that they are 
    // effectively untraceable and should be considered similarly 
    // to having a refcount of 0 (recycled)
    matteArray_t * toSweep_rootless;
    
    int gcLocked;

    // pool for value types.
    uint32_t typepool;


    matteValue_t type_empty;
    matteValue_t type_boolean;
    matteValue_t type_number;
    matteValue_t type_string;
    matteValue_t type_object;
    matteValue_t type_type;
    matteValue_t type_any;
    
    // names for all types, on-demand
    // MatteTypeData
    matteArray_t * typecode2data;
    
    matteTable_t * routeChecker;
    matteArray_t * routePather;
    matteTableIter_t * routeIter;

    matteVM_t * vm;
};


// When capturing values, the base 
// referrable of the stackframe is saved along 
// with an index into it pointing to which 
// value it is. This allows upding the real, singular 
// location of it when updating it but also allows 
// fetching copies as needed.
typedef struct {
    matteValue_t referrableSrc;
    uint32_t index;
} CapturedReferrable_t;


typedef struct {
    // any user data. Defaults to null
    void * userdata;
    
    // keys -> matteValue_t
    matteTable_t * keyvalues_string;
    matteArray_t * keyvalues_number;
    matteTable_t * keyvalues_object;
    matteValue_t keyvalue_true;
    matteValue_t keyvalue_false;
    matteTable_t * keyvalues_types;

    matteValue_t opSet;

    // has only keyvalues_number members
    int isArrayLike;
    
    // custom typecode.
    uint32_t typecode;


    
    
    // stub for functions. If null, is a normal object.
    matteBytecodeStub_t * functionstub;

    // call captured variables. Defined at function creation.
    matteArray_t * functionCaptures;

    // for type-strict functions, these are the function types 
    // asserted for args and return values.
    matteArray_t * functionTypes;

    // for function objects, a reference to the original 
    // creation context is kept. This is because referrables 
    // can track back to original creation contexts.
    matteValue_t origin;
    matteValue_t origin_referrable;
    // reference count
    uint32_t refs;
    uint32_t isRootless;
    // when refs are associated with other references,
    // a list of currently-active parents (owners of this ref) and 
    // children (the refs that consider this ref an owner) are 
    // maintained.
    //
    // Arrays of uint32_t, objectIDs to other objects.
    // references to non-objects are NOT maintained.
    matteTable_t * refChildren;
    matteTable_t * refParents;

    // root refs keep children ref chains alive.
    int isRootRef;


    // id within sorted heap.
    uint32_t heapID;
} matteObject_t;


static matteValue_t object_lookup(matteObject_t * m, matteValue_t key) {
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteValue_t out = matte_heap_new_value(key.heap);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return out;
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue_t * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            matte_value_into_copy(&out, *value);
        }
        return out;
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        matteValue_t out = matte_heap_new_value(key.heap);
        double * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], key.objectID);
        uint32_t index = (uint32_t)*d;
        if (index < matte_array_get_size(m->keyvalues_number)) {
            matte_value_into_copy(&out, matte_array_at(m->keyvalues_number, matteValue_t, index));
        }
        return out;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        matteValue_t out = matte_heap_new_value(key.heap);
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);

        if (*d) {
            matte_value_into_copy(&out, m->keyvalue_true);
        } else {
            matte_value_into_copy(&out, m->keyvalue_false);
        }
        return out;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
            matte_value_into_copy(&out, *value);
        }
        return out;
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        matteValue_t * value = matte_table_find_by_uint(m->keyvalues_types, key.objectID);
        if (value) {
            matte_value_into_copy(&out, *value);
        }
        return out;
      }
    }
    return out;
}


static void object_link_parent(matteObject_t * parent, matteObject_t * child) {
    void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    matte_table_insert_by_uint(child->refParents,   parent->heapID, p+1);
    matte_table_insert_by_uint(parent->refChildren, child->heapID,  p+1);
}

static void object_unlink_parent(matteObject_t * parent, matteObject_t * child) {
    void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    if (p == (void*)0x1) {
        matte_table_remove_by_uint(child->refParents, parent->heapID);
        matte_table_remove_by_uint(parent->refChildren, child->heapID);
    } else {
        matte_table_insert_by_uint(child->refParents,   parent->heapID, p-1);
        matte_table_insert_by_uint(parent->refChildren, child->heapID,  p-1);
    }
}

static void object_link_parent_value(matteObject_t * parent, const matteValue_t * child) {
    object_link_parent(parent,  matte_bin_fetch(child->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], child->objectID));
}

static void object_unlink_parent_value(matteObject_t * parent, const matteValue_t * child) {
    object_unlink_parent(parent,  matte_bin_fetch(child->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], child->objectID));
}


static matteValue_t * object_put_prop(matteObject_t * m, matteValue_t key, matteValue_t val) {
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteValue_t out = matte_heap_new_value(val.heap);
    matte_value_into_copy(&out, val);

    // track reference
    if (val.binID == MATTE_VALUE_TYPE_OBJECT) {
        object_link_parent_value(m, &val);
    }

    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return NULL; // SHOULD NEVER ENTER. if it would, memory leak.
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue_t * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, value);
            }
            matte_heap_recycle(*value);
            *value = out;
            return value;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert(m->keyvalues_string, str, value);
            return value;
        }
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        double * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], key.objectID);
        uint32_t index = (uint32_t)*d;
        if (index >= matte_array_get_size(m->keyvalues_number)) {
            uint32_t currentLen = matte_array_get_size(m->keyvalues_number);
            matte_array_set_size(m->keyvalues_number, index+1);
            uint32_t i;
            // new entries as empty
            for(i = currentLen; i < index+1; ++i) {
                matte_array_at(m->keyvalues_number, matteValue_t, i) = matte_heap_new_value(key.heap);
            }
        }

        matteValue_t * newV = &matte_array_at(m->keyvalues_number, matteValue_t, index);
        if (newV->binID == MATTE_VALUE_TYPE_OBJECT) {
            object_unlink_parent_value(m, newV);
        }

        matte_heap_recycle(*newV);
        *newV = out;
        return newV;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);

        if (*d) {
            if (m->keyvalue_true.binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, &m->keyvalue_true);
            }

            matte_heap_recycle(m->keyvalue_true);
            m->keyvalue_true = out;            
            return &m->keyvalue_true;
        } else {
            if (m->keyvalue_false.binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, &m->keyvalue_false);
            }

            matte_heap_recycle(m->keyvalue_false);
            m->keyvalue_false = out;            
            return &m->keyvalue_false;
        }
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, value);
            }
            matte_heap_recycle(*value);
            *value = out;
            return value;
        } else {
            object_link_parent_value(m, &key);
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert(m->keyvalues_object, &key, value);
            return value;
        }
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        matteValue_t * value = matte_table_find_by_uint(m->keyvalues_types, key.objectID);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, value);
            }
            matte_heap_recycle(*value);
            *value = out;
            return value;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert_by_uint(m->keyvalues_types, key.objectID, value);
            return value;
        }
      }
    }
    return NULL;
}

// returns a type conversion operator if it exists
static matteValue_t object_get_conv_operator(matteHeap_t * heap, matteObject_t * m, uint32_t type) {
    matteValue_t out = matte_heap_new_value(heap);
    if (m->opSet.binID == 0) return out;
    matteValue_t * operator = &m->opSet;

    matteValue_t key;
    key.heap = heap;
    key.binID = MATTE_VALUE_TYPE_TYPE;
    key.objectID = type;
    // OK: empty doesnt allocate anything
    out = matte_value_object_access(*operator, key, 0);
    return out;
}

// isDot is either 0 ([]) or 1 (.) 
// read if either 0 (write) or 1(read)
static matteValue_t object_get_access_operator(matteHeap_t * heap, matteObject_t * m, int isBracket, int read) {
    matteValue_t out = matte_heap_new_value(heap);
    if (m->opSet.binID == 0) return out;


    
    matteValue_t set = matte_value_object_access_string(m->opSet, MATTE_STR_CAST(isBracket ? "[]" : "."));
    if (set.binID) {
        if (set.binID != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_string(heap->vm, MATTE_STR_CAST("operator['[]'] and operator['.'] property must be an Object if it is set."));
        } else {
            out = matte_value_object_access_string(set, MATTE_STR_CAST(read ? "get" : "set"));
        }            
    }

    matte_heap_recycle(set);            
    return out;
}


static void * create_double() {
    return malloc(sizeof(double));
}

static void destroy_double(void * d) {free(d);}

static void * create_boolean() {
    return malloc(sizeof(int));
}
static void destroy_boolean(void * d) {free(d);}

static void * create_string() {
    return matte_string_create();
}
static void destroy_string(void * d) {matte_string_destroy(d);}

static void * create_object() {
    matteObject_t * out = calloc(1, sizeof(matteObject_t));
    out->keyvalues_string = matte_table_create_hash_matte_string();
    out->keyvalues_types  = matte_table_create_hash_pointer();
    out->keyvalues_number = matte_array_create(sizeof(matteValue_t));
    out->keyvalues_object = matte_table_create_hash_buffer(sizeof(matteValue_t));
    out->functionCaptures = matte_array_create(sizeof(CapturedReferrable_t));

    out->refChildren = matte_table_create_hash_pointer();
    out->refParents = matte_table_create_hash_pointer();
    return out;
}

static void destroy_object(void * d) {
    matteObject_t * out = d;
    matteTableIter_t * iter = matte_table_iter_create();
    for(matte_table_iter_start(iter, out->keyvalues_string);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)
    ) {
        free(matte_table_iter_get_value(iter));
    }
    for(matte_table_iter_start(iter, out->keyvalues_types);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)
    ) {
        free(matte_table_iter_get_value(iter));
    }

    matte_table_iter_destroy(iter);


    matte_table_destroy(out->keyvalues_string);
    matte_table_destroy(out->keyvalues_types);
    matte_table_destroy(out->keyvalues_object);
    matte_array_destroy(out->keyvalues_number);
    matte_array_destroy(out->functionCaptures);

    free(out);
}





matteHeap_t * matte_heap_create(matteVM_t * vm) {
    matteHeap_t * out = calloc(1, sizeof(matteHeap_t));
    out->vm = vm;
    out->sortedHeaps[MATTE_VALUE_TYPE_NUMBER] = matte_bin_create(create_double, destroy_double);
    out->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN] = matte_bin_create(create_boolean, destroy_boolean);
    out->sortedHeaps[MATTE_VALUE_TYPE_STRING] = matte_bin_create(create_string, destroy_string);
    out->sortedHeaps[MATTE_VALUE_TYPE_OBJECT] = matte_bin_create(create_object, destroy_object);
    out->typecode2data = matte_array_create(sizeof(MatteTypeData));
    out->routeChecker = matte_table_create_hash_pointer();
    out->routePather = matte_array_create(sizeof(uint32_t));
    out->routeIter = matte_table_iter_create();
    MatteTypeData dummyD = {};
    matte_array_push(out->typecode2data, dummyD);
    out->toSweep = matte_table_create_hash_pointer();
    out->toSweep_rootless = matte_array_create(sizeof(matteObject_t *));


    out->type_empty = matte_heap_new_value(out);
    out->type_boolean = matte_heap_new_value(out);
    out->type_number = matte_heap_new_value(out);
    out->type_string = matte_heap_new_value(out);
    out->type_object = matte_heap_new_value(out);
    out->type_type = matte_heap_new_value(out);
    out->type_any = matte_heap_new_value(out);

    matteValue_t dummy = {};
    matte_value_into_new_type(&out->type_empty, dummy);
    matte_value_into_new_type(&out->type_boolean, dummy);
    matte_value_into_new_type(&out->type_number, dummy);
    matte_value_into_new_type(&out->type_string, dummy);
    matte_value_into_new_type(&out->type_object, dummy);
    matte_value_into_new_type(&out->type_type, dummy);
    matte_value_into_new_type(&out->type_any, dummy);

    return out;
}

void matte_heap_destroy(matteHeap_t * h) {
    matte_heap_recycle(h->type_empty);
    matte_heap_recycle(h->type_boolean);
    matte_heap_recycle(h->type_number);
    matte_heap_recycle(h->type_string);
    matte_heap_recycle(h->type_object);
    matte_heap_recycle(h->type_type);
    matte_heap_recycle(h->type_any);

    while(!matte_table_is_empty(h->toSweep) && matte_array_get_size(h->toSweep_rootless))
        matte_heap_garbage_collect(h);

    matte_table_destroy(h->toSweep);
    matte_bin_destroy(h->sortedHeaps[MATTE_VALUE_TYPE_NUMBER]);
    matte_bin_destroy(h->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN]);
    matte_bin_destroy(h->sortedHeaps[MATTE_VALUE_TYPE_STRING]);
    matte_bin_destroy(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT]);
    free(h);
}


matteValue_t matte_heap_new_value_(matteHeap_t * h) {
    matteValue_t out;
    out.heap = h;
    out.binID = 0;
    out.objectID = 0;
    return out;
}

void matte_value_into_empty_(matteValue_t * v) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_EMPTY;
}

int matte_value_is_empty(matteValue_t v) {
    return v.binID == 0;
}


// Sets the type to a number with the given value
void matte_value_into_number_(matteValue_t * v, double val) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_NUMBER;
    double * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], &v->objectID);
    *d = val;
}


void matte_value_into_new_type_(matteValue_t * v, matteValue_t opts) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_TYPE;
    if (v->heap->typepool == 0xffffffff) {
        v->objectID = 0;
        matte_vm_raise_error_string(v->heap->vm, MATTE_STR_CAST("Type count limit reached. No more types can be created."));
    } else {
        v->objectID = ++(v->heap->typepool);
    }
    MatteTypeData data;
    if (opts.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t val = matte_value_object_access_string(opts, MATTE_STR_CAST("name"));
        if (val.binID) {
            data.name = matte_value_as_string(val);
            matte_heap_recycle(val);
        } else {
            data.name = matte_string_create_from_c_str("<anonymous type %d>", v->objectID);        
        }
        
        /// TODO: ISA. array for multiple inheritance please!
        
        
    } else {
        data.name = matte_string_create_from_c_str("<anonymous type %d>", v->objectID);
        data.isa = NULL;
    }
    matte_array_push(v->heap->typecode2data, data);
}


// Sets the type to a number with the given value
void matte_value_into_boolean_(matteValue_t * v, int val) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_BOOLEAN;
    int * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], &v->objectID);
    *d = val == 1;
}

void matte_value_into_string_(matteValue_t * v, const matteString_t * str) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_STRING;
    matteString_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], &v->objectID);
    matte_string_set(d, str);
}

void matte_value_into_new_object_ref_(matteValue_t * v) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->typecode = v->heap->type_object.objectID;
    d->refs = 1;


}


void matte_value_into_new_object_ref_typed_(matteValue_t * v, matteValue_t typeobj) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    if (typeobj.binID != MATTE_VALUE_TYPE_TYPE) {        
        matteString_t * str = matte_string_create_from_c_str("Cannot instantiate object without a Type. (given value is of type %s)", matte_value_type_name(matte_value_get_type(typeobj)));
        matte_vm_raise_error_string(v->heap->vm, str);
        matte_string_destroy(str);
        d->typecode = v->heap->type_object.objectID;
    } else {
        d->typecode = typeobj.objectID;
    }
    d->refs = 1;


}


void matte_value_into_new_object_literal_ref_(matteValue_t * v, const matteArray_t * arr) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->typecode = v->heap->type_object.objectID;
    d->refs = 1;


    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    
    for(i = 0; i < len; i+=2) {
        matteValue_t key   = matte_array_at(arr, matteValue_t, i);
        matteValue_t value = matte_array_at(arr, matteValue_t, i+1);
        
        object_put_prop(d, key, value);
    }

}


void matte_value_into_new_object_array_ref_(matteValue_t * v, const matteArray_t * args) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->typecode = v->heap->type_object.objectID;
    d->refs = 1;


    uint32_t i;
    uint32_t len = matte_array_get_size(args);


    matte_array_set_size(d->keyvalues_number, len);
    for(i = 0; i < len; ++i) {
        matteValue_t val = matte_heap_new_value(v->heap);
        matte_value_into_copy(&val, matte_array_at(args, matteValue_t, i));
        matte_array_at(d->keyvalues_number, matteValue_t, i) = val;
    }
}

matteValue_t matte_value_frame_get_named_referrable(matteVMStackFrame_t * vframe, const matteString_t * name) {
    matteHeap_t * heap = vframe->context.heap;
    if (vframe->context.binID != MATTE_VALUE_TYPE_OBJECT) return matte_heap_new_value(heap);
    matteObject_t * m = matte_bin_fetch(heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], vframe->context.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    if (!m->functionstub) return matte_heap_new_value(heap);
    // claim captures immediately.
    uint32_t i;
    uint32_t len;



    // referrables come from a history of creation contexts.
    matteValue_t context = vframe->context;
    matteValue_t contextReferrable = vframe->referrable;

    matteValue_t out = matte_heap_new_value(heap);

    while(context.binID) {
        matteObject_t * origin = matte_bin_fetch(heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], context.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(origin->refs != 0xffffffff);
        #endif
        #if MATTE_DEBUG__HEAP
            printf("Looking in args..\n");
        #endif

        len = matte_bytecode_stub_arg_count(origin->functionstub);
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__HEAP
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_bytecode_stub_get_arg_name(origin->functionstub, i)),
                    matte_string_get_c_str(name)
                );
            #endif
            if (matte_string_test_eq(matte_bytecode_stub_get_arg_name(origin->functionstub, i), name)) {
                matte_value_into_copy(&out, *matte_value_object_array_at_unsafe(contextReferrable, 1+i));
                return out;
            }
        }
        
        len = matte_bytecode_stub_local_count(origin->functionstub);
        #if MATTE_DEBUG__HEAP
            printf("Looking in locals..\n");
        #endif
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__HEAP
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_bytecode_stub_get_local_name(origin->functionstub, i)),
                    matte_string_get_c_str(name)
                );
            #endif
            if (matte_string_test_eq(matte_bytecode_stub_get_local_name(origin->functionstub, i), name)) {
                matte_value_into_copy(&out, *matte_value_object_array_at_unsafe(contextReferrable, 1+i+matte_bytecode_stub_arg_count(origin->functionstub)));
                return out;
            }
        }

        context = origin->origin;
        contextReferrable = origin->origin_referrable;
    }

    matte_vm_raise_error_string(heap->vm, MATTE_STR_CAST("Could not find named referrable!"));
    return out;
}


void matte_value_into_new_typed_function_ref_(matteValue_t * v, matteBytecodeStub_t * stub, const matteArray_t * args) {
    uint32_t i;
    uint32_t len = matte_array_get_size(args);
    // first check to see if the types are actually types.
    for(i = 0; i < len; ++i) {
        if (matte_array_at(args, matteValue_t, i).binID != MATTE_VALUE_TYPE_TYPE) {
            matteString_t * str;
            if (i == len-1) {
                str = matte_string_create_from_c_str("Function constructor with type specifiers requires those specifications to be Types. The return value specifier is not a Type.");
            } else {
                str = matte_string_create_from_c_str("Function constructor with type specifiers requires those specifications to be Types. The type specifier for Argument %d is not a Type.\n", i+1);
            }
            
            matte_vm_raise_error_string(v->heap->vm, str);
            matte_string_destroy(str);
            return;
        }
    }
    
    matte_value_into_new_function_ref(v, stub);
    matteObject_t * d = matte_bin_fetch(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v->objectID);
    d->functionTypes = matte_array_clone(args);
}

void matte_value_into_new_function_ref_(matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->refs = 1;
    d->typecode = v->heap->type_object.objectID;
    d->functionstub = stub;



    // claim captures immediately.
    uint32_t i;
    uint32_t len;
    const matteBytecodeStubCapture_t * capturesRaw = matte_bytecode_stub_get_captures(
        stub,
        &len
    );
    matteVMStackFrame_t frame;
    uint16_t fileid = matte_bytecode_stub_get_file_id(stub);

    // save origin so that others may use referrables.
    // This happens in every case EXCEPT the 0-stub function.
    if (matte_vm_get_stackframe_size(v->heap->vm)) {
        frame = matte_vm_get_stackframe(v->heap->vm, 0);

        matte_value_into_copy(&d->origin, frame.context);
        matte_value_into_copy(&d->origin_referrable, frame.referrable);
    }
    // TODO: can we speed this up?
    if (!len) return;


    // referrables come from a history of creation contexts.
    for(i = 0; i < len; ++i) {
        matteValue_t context = frame.context;
        matteValue_t contextReferrable = frame.referrable;

        while(context.binID) {
            matteObject_t * origin = matte_bin_fetch(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], context.objectID);
            #ifdef MATTE_VERIFY__HEAP_RECYCLE
                assert(origin->refs != 0xffffffff);
            #endif
            if (matte_bytecode_stub_get_id(origin->functionstub) == capturesRaw[i].stubID) {
                CapturedReferrable_t ref;
                ref.referrableSrc = matte_heap_new_value(v->heap);
                matte_value_into_copy(&ref.referrableSrc, contextReferrable);
                matte_value_object_push_lock(ref.referrableSrc);
                ref.index = capturesRaw[i].referrable;

                matte_array_push(d->functionCaptures, ref);
                break;
            }
            context = origin->origin;
            contextReferrable = origin->origin_referrable;
        }

        if (!context.binID) {
            matte_vm_raise_error_string(v->heap->vm, MATTE_STR_CAST("Could not find captured variable!"));
            CapturedReferrable_t ref;
            ref.referrableSrc = matte_heap_new_value(v->heap);
            ref.index = 0;
            matte_array_push(d->functionCaptures, ref);
        }
    }
}


matteValue_t * matte_value_object_array_at_unsafe(matteValue_t v, uint32_t index) {
    matteObject_t * d = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(d->refs != 0xffffffff);
    #endif
    return &matte_array_at(d->keyvalues_number, matteValue_t, index);
}


const matteString_t * matte_value_string_get_string_unsafe(matteValue_t v) {
    #ifdef MATTE_DEBUG 
    assert(v.binID == MATTE_VALUE_TYPE_STRING);
    #endif
    return matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);    
}


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        return m->functionstub;
    }
    return NULL;
}

matteValue_t * matte_value_get_captured_value(matteValue_t v, uint32_t index) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        if (index >= matte_array_get_size(m->functionCaptures)) return NULL;
        CapturedReferrable_t referrable = matte_array_at(m->functionCaptures, CapturedReferrable_t, index);
        return matte_value_object_array_at_unsafe(referrable.referrableSrc, referrable.index);
    }
    return NULL;
}

void matte_value_print(matteValue_t v) {
    printf(    "Matte HEAP Object:\n");
    printf(    "  Heap ID:   %d\n", v.objectID);
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        printf("  Heap Type: EMPTY\n");
        break;
      case MATTE_VALUE_TYPE_BOOLEAN: 
        printf("  Heap Type: BOOLEAN\n");
        printf("  Value:     %s\n", matte_value_as_boolean(v) ? "true" : "false");
        break;
      case MATTE_VALUE_TYPE_NUMBER: 
        printf("  Heap Type: NUMBER\n");
        printf("  Value:     %f\n", matte_value_as_number(v));
        break;
      case MATTE_VALUE_TYPE_STRING: 
        printf("  Heap Type: STRING\n");
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_as_string(v)));
        break;
      case MATTE_VALUE_TYPE_TYPE: 
        printf("  Heap Type: TYPE\n");
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_as_string(v)));
        break;

      case MATTE_VALUE_TYPE_OBJECT: {
        printf("  Heap Type: OBJECT\n");
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);            
        printf("  Refs:      %d\n", m->refs);
        printf("  Function?  %s\n", m->functionstub == NULL ? "false" : "true");
        printf("  Parents?   %s\n", matte_table_is_empty(m->refParents) ? "true" : "false");
        printf("  Children?  %s\n", matte_table_is_empty(m->refChildren) ? "true" : "false");
                            
        /*{
            printf("  String Values:\n");
            matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);            
            matteTableIter_t * iter = matte_table_iter_create();
            for( matte_table_iter_start(iter, m->keyvalues_string);
                !matte_table_iter_is_end(iter);
                 matte_table_iter_proceed(iter)) {

                matteString_t * v = matte_value_as_string(*(matteValue_t*)matte_table_iter_get_value(iter));
                printf("    \"%s\" : %s\n", 
                    matte_string_get_c_str(matte_table_iter_get_key(iter)),
                    v ? matte_string_get_c_str(v) : "<not string coercible>"
                );

            }
        }*/
        break;
      }
    }
    
    printf("\n");
    fflush(stdout);
}


void matte_value_set_object_userdata(matteValue_t v, void * userData) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        m->userdata = userData;
    }
}

void * matte_value_get_object_userdata(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        return m->userdata;
    }
    return NULL;
}



double matte_value_as_number(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert empty into a number."));
        return 0;
      case MATTE_VALUE_TYPE_TYPE: 
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot type value into a number."));
        return 0;

      case MATTE_VALUE_TYPE_NUMBER: {
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return *m;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m;
      }
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return strtod(matte_string_get_c_str(m), NULL);
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        matteValue_t operator = object_get_conv_operator(v.heap, m, v.heap->type_number.objectID);
        if (operator.binID) {
            double num = matte_value_as_number(matte_vm_call(v.heap->vm, operator, MATTE_ARRAY_CAST(&v, matteValue_t, 1), MATTE_STR_CAST("Number type conversion operator")));
            matte_heap_recycle(operator);
            return num;
        } else {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert object into a number. No 'Number' property in 'operator' object."));
        }
      }
    }
    return 0;
}

matteString_t * matte_value_as_string(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        //matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert empty into a string."));
        ///return 0;
        return matte_string_create_from_c_str("empty");

      case MATTE_VALUE_TYPE_TYPE: { 
        return matte_string_clone(matte_value_type_name(v));
      }
      case MATTE_VALUE_TYPE_NUMBER: {
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return matte_string_create_from_c_str("%g", *m);
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m ? matte_string_create_from_c_str("%s", "true") : matte_string_create_from_c_str("%s", "false");
      }
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return matte_string_clone(m);
      }

      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        matteValue_t operator = object_get_conv_operator(v.heap, m, v.heap->type_string.objectID);
        if (operator.binID) {
            matteString_t * out = matte_value_as_string(matte_vm_call(v.heap->vm, operator, MATTE_ARRAY_CAST(&v, matteValue_t, 1), MATTE_STR_CAST("String type conversion operator")));
            matte_heap_recycle(operator);
            return out;
        } else {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Object has no valid conversion to string."));
            return matte_string_create_from_c_str("");
        }
      }
    }
    return matte_string_create();
}


matteValue_t matte_value_to_type(matteValue_t v, matteValue_t t) {
    matteValue_t out = matte_heap_new_value(v.heap);
    switch(t.objectID) {
      case 1: { // empty
        if (v.binID != MATTE_VALUE_TYPE_EMPTY) {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("It is an error to convert any non-empty value to the Empty type."));
        } else {
            matte_value_into_copy(&out, v);
        }

        break;
      }

      case 6: {
        if (v.binID != MATTE_VALUE_TYPE_TYPE) {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("It is an error to convert any non-Type value to a Type."));
        } else {
            matte_value_into_copy(&out, v);
        }
        break;
      }

      
      case 2: {
        matte_value_into_boolean(&out, matte_value_as_boolean(v));
        break;
      }
      case 3: {
        matte_value_into_number(&out, matte_value_as_number(v));
        break;
      }

      case 4: {
        matteString_t * str = matte_value_as_string(v);
        matte_value_into_string(&out, str);
        matte_string_destroy(str);
        break;
      }

      case 5: {
        if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
            matte_value_into_copy(&out, v);
            break;
        } else {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert value to Object type."));        
        }
      
      }
      
      default:;// TODO: custom types.
      
      
    
    }
    return out;
}

int matte_value_as_boolean(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_TYPE: {
        return 1;
      }
      case MATTE_VALUE_TYPE_NUMBER: {
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return *m != 0.0;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m;
      }
      case MATTE_VALUE_TYPE_STRING: {
        // non-empty value
        return 1;
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        matteValue_t operator = object_get_conv_operator(v.heap, m, v.heap->type_boolean.objectID);
        if (operator.binID) {
            int out = matte_value_as_boolean(matte_vm_call(v.heap->vm, operator, MATTE_ARRAY_CAST(&v, matteValue_t, 1), MATTE_STR_CAST("Boolean type conversion operator")));
            matte_heap_recycle(operator);
            return out;
        } else {
            // non-empty
            return 1;
        }
      }
    }
    return 0;
}

// returns whether the value is callable.
int matte_value_is_callable(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_TYPE) return 1;
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return 0;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif    
    return (m->functionstub != NULL) + (m->functionTypes != NULL);;    
}

int matte_value_object_function_pre_typecheck_unsafe(matteValue_t v, const matteArray_t * arr) {
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);

    #ifdef MATTE_DEBUG__HEAP
        assert(len == matte_array_get_size(m->functionTypes) - 1);    
    #endif    

    for(i = 0; i < len; ++i) {
        if (!matte_value_isa(
            matte_array_at(arr, matteValue_t, i),
            matte_array_at(m->functionTypes, matteValue_t, i)
        )) {
            matteString_t * err = matte_string_create_from_c_str(
                "Argument %d (type: %s) is not of required type %s",
                i+1,
                matte_string_get_c_str(matte_value_type_name(matte_value_get_type(matte_array_at(arr, matteValue_t, i)))),
                matte_string_get_c_str(matte_value_type_name(matte_array_at(m->functionTypes, matteValue_t, i)))
            );
            matte_vm_raise_error_string(v.heap->vm, err);
            matte_string_destroy(err);
            return 0;
        }
    }
    
    return 1;
}

void matte_value_object_function_post_typecheck_unsafe(matteValue_t v, matteValue_t ret) {
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    if (!matte_value_isa(ret, matte_array_at(m->functionTypes, matteValue_t, matte_array_get_size(m->functionTypes) - 1))) {
        matteString_t * err = matte_string_create_from_c_str(
            "Return value (type: %s) is not of required type %s.",
            matte_string_get_c_str(matte_value_type_name(matte_value_get_type(ret))),
            matte_string_get_c_str(matte_value_type_name(matte_array_at(m->functionTypes, matteValue_t, matte_array_get_size(m->functionTypes) - 1)))

        );
        matte_vm_raise_error_string(v.heap->vm, err);
    }  
}


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteValue_t v, matteValue_t key, int isBracketAccess) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_type_name(matte_value_get_type(v)))
        );
        matte_vm_raise_error_string(v.heap->vm, err);
        return matte_heap_new_value(v.heap);
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteValue_t accessor = object_get_access_operator(v.heap, m, isBracketAccess, 1);
    if (accessor.binID) {
        matteValue_t args[1] = {
            key
        };
        matteValue_t a = matte_vm_call(v.heap->vm, accessor, MATTE_ARRAY_CAST(args, matteValue_t, 1), NULL);
        matte_heap_recycle(accessor);
        return a;
    } else {
        return object_lookup(m, key);
    }
}


matteValue_t matte_value_object_access_string(matteValue_t v, const matteString_t * key) {
    matteValue_t keyO = matte_heap_new_value(v.heap);
    matte_value_into_string(&keyO, key);
    matteValue_t result = matte_value_object_access(v, keyO, 0);
    matte_heap_recycle(keyO);
    return result;
}

matteValue_t matte_value_object_access_index(matteValue_t v, uint32_t index) {
    matteValue_t keyO = matte_heap_new_value(v.heap);
    matte_value_into_number(&keyO, index);
    matteValue_t result = matte_value_object_access(v, keyO, 1);
    matte_heap_recycle(keyO);
    return result;
}


void matte_value_object_push_lock(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    m->isRootRef++;
}

void matte_value_object_pop_lock(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    if (m->isRootRef)
        m->isRootRef--;
}



matteValue_t matte_value_object_keys(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(v.heap);
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->keyvalues_number);
    if (len) {
        for(i = 0; i < len; ++i) {
            matteValue_t key = matte_heap_new_value(v.heap);
            matte_value_into_number(&key, i);
            matte_array_push(keys, key);
        }
    }

    // then string
    if (!matte_table_is_empty(m->keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key = matte_heap_new_value(v.heap);
            matte_value_into_string(&key, matte_table_iter_get_key(iter));
            matte_array_push(keys, key);                
        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = *(matteValue_t*)matte_table_iter_get_value(iter);
            matteValue_t key = matte_heap_new_value(v.heap);
            matte_value_into_copy(&key, *(matteValue_t*)matte_table_iter_get_key(iter));
            matte_array_push(keys, key);    
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->keyvalue_true.binID) {
        matteValue_t key = matte_heap_new_value(v.heap);
        matte_value_into_boolean(&key, 1);
        matte_array_push(keys, key);
    }
    // false
    if (m->keyvalue_false.binID) {
        matteValue_t key = matte_heap_new_value(v.heap);
        matte_value_into_boolean(&key, 0);
        matte_array_push(keys, key);
    }
    
    // then types
    if (!matte_table_is_empty(m->keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key;
            key.heap = v.heap;
            key.binID = MATTE_VALUE_TYPE_TYPE;
            key.objectID = matte_table_iter_get_key_uint(iter);

            matte_array_push(keys, key);                
        }
        matte_table_iter_destroy(iter);
    }
    
    val = matte_heap_new_value(v.heap);
    matte_value_into_new_object_array_ref(&val, keys);
    matte_array_destroy(keys);
    return val;
}

matteValue_t matte_value_object_values(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(v.heap);
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteArray_t * vals = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->keyvalues_number);
    if (len) {
        for(i = 0; i < len; ++i) {
            val = matte_heap_new_value(v.heap);
            matte_value_into_copy(&val, matte_array_at(m->keyvalues_number, matteValue_t, i));
            matte_array_push(vals, val);
        }
    }

    // then string
    if (!matte_table_is_empty(m->keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(v.heap);
            matte_value_into_copy(&val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);                
        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(v.heap);
            matte_value_into_copy(&val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);    
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->keyvalue_true.binID) {
        val = matte_heap_new_value(v.heap);
        matte_value_into_copy(&val, m->keyvalue_true);
        matte_array_push(vals, val);
    }
    // false
    if (m->keyvalue_false.binID) {
        val = matte_heap_new_value(v.heap);
        matte_value_into_copy(&val, m->keyvalue_false);
        matte_array_push(vals, val);
    }
    
    // then types
    if (!matte_table_is_empty(m->keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(v.heap);
            matte_value_into_copy(&val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);                
        }
        matte_table_iter_destroy(iter);
    }

    val = matte_heap_new_value(v.heap);
    matte_value_into_new_object_array_ref(&val, vals);
    matte_array_destroy(vals);
    return val;
}


uint32_t matte_value_object_get_key_count(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return 0;
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    uint32_t total = 
        matte_array_get_size(m->keyvalues_number) +
        (m->keyvalue_true.binID?1:0) +
        (m->keyvalue_false.binID?1:0);

    // maybe maintain through get/set/remove prop?
    if (!matte_table_is_empty(m->keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }


    if (!matte_table_is_empty(m->keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }
    
    if (!matte_table_is_empty(m->keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }
    return total;    
}


void matte_value_object_remove_key(matteValue_t v, matteValue_t key) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return;
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue_t * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            matte_table_remove(m->keyvalues_string, str);
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, value);                
            }
            matte_heap_recycle(*value);
            free(value);
        }
        return;
      }

      case MATTE_VALUE_TYPE_TYPE: {
        matteValue_t * value = matte_table_find_by_uint(m->keyvalues_string, key.objectID);
        if (value) {
            matte_table_remove_by_uint(m->keyvalues_string, key.objectID);
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, value);                
            }
            matte_heap_recycle(*value);
            free(value);
        }
        return;
      }

      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        double * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], key.objectID);
        uint32_t index = (uint32_t)*d;
        if (index >= matte_array_get_size(m->keyvalues_number)) {
            return;
        }
        matteValue_t * value = &matte_array_at(m->keyvalues_number, matteValue_t, index);
        if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
            object_unlink_parent_value(m, value);                
        }

        matte_heap_recycle(*value);
        matte_array_remove(m->keyvalues_number, index);
        return;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);
        if (!d) return;

        if (*d) {
            if (m->keyvalue_true.binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, &m->keyvalue_true);                
            }
            matte_heap_recycle(m->keyvalue_true);
            m->keyvalue_true = matte_heap_new_value(v.heap);            
        } else {
            if (m->keyvalue_false.binID == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(m, &m->keyvalue_false);                
            }
            matte_heap_recycle(m->keyvalue_false);
            m->keyvalue_false = matte_heap_new_value(v.heap);               
        }
        return;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT) {
                    object_unlink_parent_value(m, value);                
            }
            matte_heap_recycle(*value);
            matte_table_remove(m->keyvalues_object, &key);
        }
        return;
      }
    }

}

void matte_value_object_foreach(matteValue_t v, matteValue_t func) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT || !matte_value_is_callable(func)) {
        return;
    }

    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif

    // foreach operator
    if (m->opSet.binID) {
        matteValue_t set = matte_value_object_access_string(m->opSet, MATTE_STR_CAST("foreach"));

        if (set.binID) {
            v = matte_vm_call(v.heap->vm, set, matte_array_empty(), MATTE_STR_CAST("'foreach' operator"));
            if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
                // raise error
            } else {
                m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
            }
        }
    }


    matteValue_t args[] = {
        matte_heap_new_value(v.heap),
        matte_heap_new_value(v.heap)
    };

    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->keyvalues_number);
    if (len) {
        matte_value_into_number(&args[0], 0);
        double * quickI = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], args[0].objectID);

        for(i = 0; i < len; ++i) {
            args[1] = matte_array_at(m->keyvalues_number, matteValue_t, i);
            *quickI = i;
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));
            matte_heap_recycle(r);
        }
        matte_heap_recycle(args[0]);
    }

    // then string
    if (!matte_table_is_empty(m->keyvalues_string)) {
        matte_value_into_string(&args[0], MATTE_STR_CAST(""));
        matteString_t * quickI = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], args[0].objectID);
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            matte_string_set(quickI, (matteString_t*)matte_table_iter_get_key(iter));
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));
            matte_heap_recycle(r);
        }
        matte_heap_recycle(args[0]);
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0] = *(matteValue_t*)matte_table_iter_get_value(iter);
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));
            matte_heap_recycle(r);
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->keyvalue_true.binID) {
        matte_value_into_boolean(&args[0], 1);
        args[1] = m->keyvalue_true;
        matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));        
        matte_heap_recycle(args[0]);
        matte_heap_recycle(r);
    }
    // false
    if (m->keyvalue_false.binID) {
        matte_value_into_boolean(&args[0], 1);
        args[1] = m->keyvalue_false;
        matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));        
        matte_heap_recycle(r);
        matte_heap_recycle(args[0]);
    }

    if (!matte_table_is_empty(m->keyvalues_types)) {
        matte_heap_recycle(args[0]);
        args[0].binID = MATTE_VALUE_TYPE_TYPE;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].objectID = matte_table_iter_get_key_uint(iter);
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2), MATTE_STR_CAST("'foreach' inner function"));
            matte_heap_recycle(r);
        }
        matte_heap_recycle(args[0]);
        matte_table_iter_destroy(iter);
    }

}

matteValue_t matte_value_subset(matteValue_t v, uint32_t from, uint32_t to) {
    matteValue_t out = matte_heap_new_value(v.heap);
    if (from > to) return out;

    switch(v.binID) {
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif


        uint32_t curlen = matte_array_get_size(m->keyvalues_number);
        if (from >= curlen || to >= curlen) return out;



        matte_value_into_new_object_array_ref(
            &out,
            MATTE_ARRAY_CAST(
                ((matteValue_t *)matte_array_get_data(m->keyvalues_number))+from,
                matteValue_t,
                (to-from)+1
            )
        );

        break;
      }


      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        uint32_t curlen = matte_string_get_length(str);
        if (from >= curlen || to >= curlen) return out;

        matte_value_into_string(&out, matte_string_get_substr(str, from, to));
      }


    }
    return out;
}

matteValue_t matte_value_object_array_to_string_unsafe(matteValue_t v) {
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteHeap_t * heap = v.heap;

    matteValue_t out = matte_heap_new_value(v.heap);
    matte_value_into_string(&out, MATTE_STR_CAST(""));
    matteString_t * str = matte_bin_fetch(heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], out.objectID);
    uint32_t len = matte_array_get_size(m->keyvalues_number);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        matteValue_t * vn = &matte_array_at(m->keyvalues_number, matteValue_t, i);        
        if (vn->binID == MATTE_VALUE_TYPE_NUMBER) {
            matte_string_append_char(
                str,
                (int)(*(double*)matte_bin_fetch(heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], vn->objectID))
            );
        }
    }
    
    return out;
}


void matte_value_object_set_operator(matteValue_t v, matteValue_t opObject) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot assign operator set to something that isnt an object."));
        return;
    }
    
    if (opObject.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot assign operator set that isn't an object."));
        return;
    }
    
    if (opObject.objectID == v.objectID) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot assign operator set as its own object."));
        return;    
    }
    
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif    
    matte_value_into_copy(&m->opSet, opObject);
}

const matteValue_t * matte_value_object_get_operator_unsafe(matteValue_t v) {
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    if (m->opSet.binID) return &m->opSet;
    return NULL;
}


const matteValue_t * matte_value_object_set(matteValue_t v, matteValue_t key, matteValue_t value, int isBracket) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot set property on something that isnt an object."));
        return NULL;
    }
    if (key.binID == 0) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot set property with an empty key"));
        return NULL;
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff);
    #endif
    matteValue_t assigner = object_get_access_operator(v.heap, m, isBracket, 0);
    if (assigner.binID) {
        matteValue_t args[2] = {
            key, value
        };
        matteValue_t r = matte_vm_call(v.heap->vm, assigner, MATTE_ARRAY_CAST(args, matteValue_t, 2), NULL);
        matte_heap_recycle(assigner);
        if (r.binID == MATTE_VALUE_TYPE_BOOLEAN && !matte_value_as_boolean(r)) {
            matte_heap_recycle(r);
            return NULL;
        } else {
            matte_heap_recycle(r);
            return object_put_prop(m, key, value); 
        }
    } else {
        return object_put_prop(m, key, value);
    }
}


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy_(matteValue_t * to, matteValue_t from) {

    if (to->objectID == from.objectID &&
        to->binID    == from.binID) return;
        

    switch(from.binID) {
      case MATTE_VALUE_TYPE_EMPTY:
        *to = from;
        return;

      case MATTE_VALUE_TYPE_NUMBER: 
        matte_value_into_number(to, *(double*)matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], from.objectID));
        return;

      case MATTE_VALUE_TYPE_TYPE: 
        *to = from;
        return;

      case MATTE_VALUE_TYPE_BOOLEAN: 
        matte_value_into_boolean(to, *(int*)matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], from.objectID));
        return;
      case MATTE_VALUE_TYPE_STRING: 
        matte_value_into_string(to, matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], from.objectID));
        return;
      case MATTE_VALUE_TYPE_OBJECT: {
        matte_heap_recycle(*to);
        matteObject_t * m = matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], from.objectID);
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif
        m->refs++;

        *to = from;
        return;
      }
    }
}

uint32_t matte_value_type_get_typecode(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_TYPE) return 0;
    return v.objectID;
}


int matte_value_isa(matteValue_t v, matteValue_t typeobj) {
    if (typeobj.binID != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("VM error: cannot query isa() with a non Type value."));
        return 0;
    }
    if (typeobj.objectID == v.heap->type_any.objectID) return 1;
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_value_get_type(v).objectID == typeobj.objectID;
    } else {
        // TODO: more complex stuff for custom type
        return matte_value_get_type(v).objectID == typeobj.objectID;
    }
}

const matteString_t * matte_value_type_name(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("VM error: cannot get type name of non Type value."));
        return MATTE_STR_CAST("");
    }
    switch(v.objectID) {
      case 1:    return MATTE_STR_CAST("Empty");
      case 2:  return MATTE_STR_CAST("Boolean");
      case 3:   return MATTE_STR_CAST("Number");
      case 4:   return MATTE_STR_CAST("String");
      case 5:   return MATTE_STR_CAST("Object");
      case 6:     return MATTE_STR_CAST("Type");
      default: { 
        if (v.objectID >= matte_array_get_size(v.heap->typecode2data)) {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("VM error: no such type exists..."));                    
        } else
            return matte_array_at(v.heap->typecode2data, MatteTypeData, v.objectID).name;
      }
    }

    return MATTE_STR_CAST("");

}



const matteValue_t * matte_heap_get_empty_type(matteHeap_t * h) {
    return &h->type_empty;
}
const matteValue_t * matte_heap_get_boolean_type(matteHeap_t * h) {
    return &h->type_boolean;
}
const matteValue_t * matte_heap_get_number_type(matteHeap_t * h) {
    return &h->type_number;
}
const matteValue_t * matte_heap_get_string_type(matteHeap_t * h) {
    return &h->type_string;
}
const matteValue_t * matte_heap_get_object_type(matteHeap_t * h) {
    return &h->type_object;
}
const matteValue_t * matte_heap_get_type_type(matteHeap_t * h) {
    return &h->type_type;
}
const matteValue_t * matte_heap_get_any_type(matteHeap_t * h) {
    return &h->type_any;
}

matteValue_t matte_value_get_type(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY:   return v.heap->type_empty;
      case MATTE_VALUE_TYPE_BOOLEAN: return v.heap->type_boolean;
      case MATTE_VALUE_TYPE_NUMBER: return v.heap->type_number;
      case MATTE_VALUE_TYPE_STRING: return v.heap->type_string;
      case MATTE_VALUE_TYPE_TYPE: return v.heap->type_type;
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        matteValue_t out;
        out.binID = MATTE_VALUE_TYPE_TYPE;
        out.objectID = m->typecode;
        out.heap = v.heap;
        return out;
      }

    }
    return v.heap->type_empty;
}




// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle_(
    matteValue_t v
#ifdef MATTE_DEBUG__HEAP
    ,const char * file_,
    int line_
#endif    
) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        // already handled as part of a rootless sweep.

        if (m->isRootless) return;

        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            assert(m->refs != 0xffffffff);
        #endif 
        if (m->refs) {
            m->refs--;
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_out(v, file_, line_);            
            #endif

            // should guarantee no repeats
            //if (m->refs == 0)
            // any time a ref gets decremented, we'll need to check if it has lost a path to root.

            matte_table_insert(v.heap->toSweep, m, (void*)0x1);                
        }
    } else if (v.binID && v.binID != MATTE_VALUE_TYPE_TYPE) {
        #ifdef MATTE_DEBUG__HEAP
            matte_heap_track_out(v, file_, line_);            
        #endif
        matte_bin_recycle(v.heap->sortedHeaps[v.binID], v.objectID);
    }
    #ifdef MATTE_DEBUG__HEAP
    else if (v.binID) {
        matte_heap_track_out(v, file_, line_);            
    }    
    #endif
}

// checks to see if the object has a reference back to its parents
static int matte_object_check_ref_path_root(matteHeap_t * h, matteObject_t * m) {
    if (m->isRootRef) return 1;
    matte_array_push(h->routePather, m->heapID);
    matte_table_insert_by_uint(h->routeChecker, m->heapID, (void*)0x1);
    matteTableIter_t * iter = h->routeIter;
    int foundRoot = 0;
    uint32_t i;
    uint32_t len;
    uint32_t size = 1;
    while(size) {
        uint32_t currentID = matte_array_at(h->routePather, uint32_t, --size);
        matte_array_set_size(h->routePather, size);
        matteObject_t * current = matte_bin_fetch(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], currentID);
        if (current->isRootRef) {
            foundRoot = 1;
            break;
        }


        for(matte_table_iter_start(iter, current->refParents);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)) {
            uint32_t nc = matte_table_iter_get_key_uint(iter);
            if (!matte_table_find_by_uint(h->routeChecker, nc)) {
                matte_table_insert_by_uint(h->routeChecker, nc, (void*)0x1);
                matte_array_push(h->routePather, nc);
                size++;
            }                        
        }
        for(matte_table_iter_start(iter, current->refChildren);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)) {
            uint32_t nc = matte_table_iter_get_key_uint(iter);
            if (!matte_table_find_by_uint(h->routeChecker, nc)) {
                matte_table_insert_by_uint(h->routeChecker, nc, (void*)0x1);
                matte_array_push(h->routePather, nc);
                size++;
            }                        
        }



    }
    matte_array_set_size(h->routePather, 0);
    matte_table_clear(h->routeChecker);
    return foundRoot;
}

// add all objects in the group / cycle to the special rootless bin.
// There they will be handled safely (and separately) from the 
// normal sweep.
static void matte_object_group_add_to_rootless_bin(matteHeap_t * h, matteObject_t * m) {
    matte_array_push(h->routePather, m->heapID);
    matte_table_insert_by_uint(h->routeChecker, m->heapID, (void*)0x1);
    matteTableIter_t * iter = h->routeIter;
    uint32_t i;
    uint32_t len;
    uint32_t size = 1;
    while(size) {
        uint32_t currentID = matte_array_at(h->routePather, uint32_t, --size);
        matte_array_set_size(h->routePather, size);
        matteObject_t * current = matte_bin_fetch(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], currentID);

        current->isRootless = 1;
        matte_array_push(h->toSweep_rootless, current);


        for(matte_table_iter_start(iter, current->refParents);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)) {
            uint32_t nc = matte_table_iter_get_key_uint(iter);
            if (!matte_table_find_by_uint(h->routeChecker, nc)) {
                matte_table_insert_by_uint(h->routeChecker, nc, (void*)0x1);
                matte_array_push(h->routePather, nc);
                size++;
            }                        
        }

        for(matte_table_iter_start(iter, current->refChildren);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)) {
            uint32_t nc = matte_table_iter_get_key_uint(iter);
            if (!matte_table_find_by_uint(h->routeChecker, nc)) {
                matte_table_insert_by_uint(h->routeChecker, nc, (void*)0x1);
                matte_array_push(h->routePather, nc);
                size++;
            }                        
        }

    }

    #ifdef MATTE_DEBUG__HEAP
        if (len) {
            printf("CYCLE/GROUP FOUND. Size:%d \n", matte_array_get_size(h->routePather));
        }
    #endif
    matte_array_set_size(h->routePather, 0);
    matte_table_clear(h->routeChecker);
}


static void object_cleanup(matteHeap_t * h, matteObject_t * m) {
    matteValue_t * scriptFinalizer = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("finalizer"));
    if (scriptFinalizer) {
        matteValue_t object = matte_heap_new_value(h);
        object.binID = MATTE_VALUE_TYPE_OBJECT;
        object.objectID = m->heapID;

        // should be fine despite ref count being 0
        matte_vm_call(h->vm, *scriptFinalizer, MATTE_ARRAY_CAST(&object, matteValue_t, 1), MATTE_STR_CAST("Finalizer"));
    }
    #ifdef MATTE_VERIFY__HEAP_RECYCLE
        assert(m->refs != 0xffffffff && "Object was supposed to have been dead.");
    #endif
    matteTableIter_t * iter = matte_table_iter_create();


    // might have been saved by finalizer
    if (!(m->refs != 0 && matte_object_check_ref_path_root(h, m))) {
        #ifdef MATTE_DEBUG__HEAP
            printf("--RECYCLING OBJECT %d\n", m->heapID);
        #endif

        #ifdef MATTE_DEBUG__HEAP

            {
                uint32_t i;
                while(m->refs) {
                    matteValue_t v;
                    v.objectID = m->heapID;
                    v.binID = MATTE_VALUE_TYPE_OBJECT;
                    v.heap = h;
                    matte_heap_track_out(v, "<nothing>", -1);
                    m->refs--;
                }
            }
        #endif
        #ifdef MATTE_VERIFY__HEAP_RECYCLE
            m->refs = 0xffffffff;
        #else 
            m->refs = 0;
        #endif
        // watch out!!!! might have been re-entered in toSweep if finalizer 
        // logic saved then re-killed it... whoops...
        matte_table_remove(h->toSweep, m);



        m->isRootless = 0;


        if (!matte_table_is_empty(m->refChildren)) {


            for(matte_table_iter_start(iter, m->refChildren);
                !matte_table_iter_is_end(iter);
                matte_table_iter_proceed(iter)
            ) {
                uint32_t nc = matte_table_iter_get_key_uint(iter);
                object_unlink_parent(m, matte_bin_fetch(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], nc));
            }
            matte_table_iter_destroy(iter);
            matte_table_clear(m->refChildren);
        }

        if (!matte_table_is_empty(m->refParents)) {


            for(matte_table_iter_start(iter, m->refParents);
                !matte_table_iter_is_end(iter);
                matte_table_iter_proceed(iter)
            ) {
                uint32_t nc = matte_table_iter_get_key_uint(iter);
                object_unlink_parent(matte_bin_fetch(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], nc), m);
            }
            matte_table_iter_destroy(iter);
            matte_table_clear(m->refParents);
        }
        m->isRootRef = 0;


        // clean up object;
        #ifndef MATTE_VERIFY__HEAP_RECYCLE
            m->functionstub = NULL;
            m->userdata = NULL;
        #endif
        
        if (m->opSet.binID) {
            matte_heap_recycle(m->opSet);
            m->opSet.binID = 0;
        }


        if (m->keyvalue_true.binID) {
            matte_heap_recycle(m->keyvalue_true);
            m->keyvalue_true.binID = 0;
        }
        if (m->keyvalue_false.binID) {
            matte_heap_recycle(m->keyvalue_false);
            m->keyvalue_false.binID = 0;
        }
        if (matte_array_get_size(m->keyvalues_number)) {
            uint32_t n;
            uint32_t subl = matte_array_get_size(m->keyvalues_number);
            for(n = 0; n < subl; ++n) {
                matte_heap_recycle(matte_array_at(m->keyvalues_number, matteValue_t, n));
            }
            matte_array_set_size(m->keyvalues_number, 0);
        }
        if (m->functionstub) {
            uint32_t n;
            uint32_t subl = matte_array_get_size(m->functionCaptures);
            for(n = 0; n < subl; ++n) {
                matte_value_object_pop_lock(matte_array_at(m->functionCaptures, CapturedReferrable_t, n).referrableSrc);
                matte_heap_recycle(matte_array_at(m->functionCaptures, CapturedReferrable_t, n).referrableSrc);
            }
            matte_array_set_size(m->functionCaptures, 0);
            matte_heap_recycle(m->origin);
            m->origin.binID = 0;
            matte_heap_recycle(m->origin_referrable);
            m->origin_referrable.binID = 0;
            if (m->functionTypes) {
                matte_array_destroy(m->functionTypes);
                m->functionTypes = NULL;
            }
        }
        if (!matte_table_is_empty(m->keyvalues_string)) {
            for(matte_table_iter_start(iter, m->keyvalues_string);
                matte_table_iter_is_end(iter) == 0;
                matte_table_iter_proceed(iter)
            ) {
                matteValue_t * v = matte_table_iter_get_value(iter);
                matte_heap_recycle(*v);
                free(v);
            }
            matte_table_clear(m->keyvalues_string);
        }
        
        if (!matte_table_is_empty(m->keyvalues_types)) {
            matteTableIter_t * iter = matte_table_iter_create();


            for(matte_table_iter_start(iter, m->keyvalues_types);
                matte_table_iter_is_end(iter) == 0;
                matte_table_iter_proceed(iter)
            ) {
                matteValue_t * v = matte_table_iter_get_value(iter);
                matte_heap_recycle(*v);
                free(v);
            }
            matte_table_clear(m->keyvalues_types);
        }
        if (!matte_table_is_empty(m->keyvalues_object)) {
            for(matte_table_iter_start(iter, m->keyvalues_object);
                matte_table_iter_is_end(iter) == 0;
                matte_table_iter_proceed(iter)
            ) {
                matteValue_t * v = matte_table_iter_get_value(iter);
                matte_heap_recycle(*v);
                free(v);
            }
            matte_table_clear(m->keyvalues_object);
        }
        #ifndef MATTE_VERIFY__HEAP_RECYCLE
            matte_bin_recycle(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], m->heapID);
        #endif
    }
    matte_table_iter_destroy(iter);
}

void matte_heap_garbage_collect(matteHeap_t * h) {
    if (h->gcLocked) return;
    h->gcLocked = 1;
    // transfer to an array juuuust in case the 
    // finalizers mark additional objects for GCing
    matteArray_t * sweep = matte_array_create(sizeof(matteObject_t *));
    matteTableIter_t * iter = matte_table_iter_create();
    for(matte_table_iter_start(iter, h->toSweep);
        matte_table_iter_is_end(iter) == 0;
        matte_table_iter_proceed(iter)
    ) {
        const matteObject_t * v = matte_table_iter_get_key(iter);
        matte_array_push(sweep, v);
    }

    matte_table_clear(h->toSweep);


    
    uint32_t i;
    uint32_t len = matte_array_get_size(sweep);
    #ifdef MATTE_DEBUG__HEAP
        if (len) {
            printf("BEGINNING SWEEP\n");
        }
    #endif

    for(i = 0; i < len; ++i) {
        matteObject_t * m = matte_array_at(sweep, matteObject_t *, i);
        if (m->isRootless) continue;

        // might have been "saved" elsewhere.
        if (m->refs == 0) {
            object_cleanup(h, m);
        } else if (!matte_object_check_ref_path_root(h, m)) {
            matte_object_group_add_to_rootless_bin(h, m);
        }
    }
    matte_table_iter_destroy(iter);
    matte_array_destroy(sweep);

    // then we also check the rootless sweeping bin.
    // we limit the size to 100 objects per iteration.
    //#define ROOTLESS_SWEEP_LIMIT_COUNT 100

    len = matte_array_get_size(h->toSweep_rootless);
    if (len) {
        //if (len > ROOTLESS_SWEEP_LIMIT_COUNT) len = ROOTLESS_SWEEP_LIMIT_COUNT;
        uint32_t iter = matte_array_get_size(h->toSweep_rootless)-1;
        for(i = 0; i < len; ++i) {
            object_cleanup(h, matte_array_at(h->toSweep_rootless, matteObject_t *, iter--));
        }
        matte_array_set_size(
            h->toSweep_rootless,
            matte_array_get_size(h->toSweep_rootless) - len
        );
    }
    
    h->gcLocked = 0;
}





#ifdef MATTE_DEBUG__HEAP 


typedef struct {
    uint32_t refid;
    int refCount;
    matteArray_t * historyFiles;
    matteArray_t * historyLines;
    matteArray_t * historyIncr;
} MatteHeapTrackInfo_Instance;

typedef struct {
    matteTable_t * values[MATTE_VALUE_TYPE_TYPE+1];
    int resolved;
    int unresolved;
} MatteHeapTrackInfo;


static MatteHeapTrackInfo * heap_track_info = NULL;


static void matte_heap_track_initialize() {
    heap_track_info = calloc(1, sizeof(MatteHeapTrackInfo));
    heap_track_info->values[MATTE_VALUE_TYPE_BOOLEAN] = matte_table_create_hash_pointer();
    heap_track_info->values[MATTE_VALUE_TYPE_NUMBER] = matte_table_create_hash_pointer();
    heap_track_info->values[MATTE_VALUE_TYPE_STRING] = matte_table_create_hash_pointer();
    heap_track_info->values[MATTE_VALUE_TYPE_OBJECT] = matte_table_create_hash_pointer();
    heap_track_info->values[MATTE_VALUE_TYPE_TYPE] = matte_table_create_hash_pointer();
}

static void matte_heap_report_val(matteTable_t * t) {
    matteTableIter_t * iter = matte_table_iter_create();
    for(matte_table_iter_start(iter, t);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        MatteHeapTrackInfo_Instance * inst = matte_table_iter_get_value(iter);
        printf("refid: %d\n", inst->refid);
        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            printf(
                "%s %s:%d\n", 
                matte_array_at(inst->historyIncr, int, i) ? "++" : "--",
                matte_array_at(inst->historyFiles, char *, i),
                matte_array_at(inst->historyLines, int, i)
            );
        }
    }
}

matteValue_t matte_heap_track_in(matteValue_t val, const char * file, int line) {
    if (!heap_track_info) {
        matte_heap_track_initialize();
    }
    if (!val.binID) return val;
    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[val.binID], val.objectID);
    if (!inst) {
        inst = calloc(sizeof(MatteHeapTrackInfo_Instance), 1);
        inst->historyFiles = matte_array_create(sizeof(char *));
        inst->historyLines = matte_array_create(sizeof(int));
        inst->historyIncr  = matte_array_create(sizeof(int));
        matte_table_insert_by_uint(heap_track_info->values[val.binID], val.objectID, inst);
        heap_track_info->unresolved++;
        inst->refid = val.objectID;
    }

    char * fdup = strdup(file);
    int t = 1;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);
    inst->refCount++;
    return val;
}
void matte_heap_track_out(matteValue_t val, const char * file, int line) {
    if (!heap_track_info) {
        matte_heap_track_initialize();
    }
    if (!val.binID) return;

    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[val.binID], val.objectID);
    if (!inst) {
        assert(!"If this assertion fails, something has used a stray / uninitialized value and tried to distroy it.");
    }
    char * fdup = strdup(file);
    int t = 0;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);

    inst->refCount--;
    // reference is complete. REMOVE!!!
    if (!inst->refCount) {
        matte_array_destroy(inst->historyLines);
        matte_array_destroy(inst->historyIncr);
        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            free(matte_array_at(inst->historyFiles, char *, i));
        }
        matte_array_destroy(inst->historyFiles);
        matte_table_remove_by_uint(heap_track_info->values[val.binID], val.objectID);
        heap_track_info->resolved++;
        heap_track_info->unresolved--;
        free(inst);
    }
}

// prints a report of the heap if any references have not 
// 
int matte_heap_report() {
    if (!heap_track_info) return 0;
    if (!heap_track_info->unresolved) return 0;

    printf("--=-- Matte Heap Value Tracker --=--\n");
    printf("|     UNFREED VALUES DETECTED      |\n");
    printf("Boolean values:\n"); matte_heap_report_val(heap_track_info->values[MATTE_VALUE_TYPE_BOOLEAN]);
    printf("Number values:\n"); matte_heap_report_val(heap_track_info->values[MATTE_VALUE_TYPE_NUMBER]);
    printf("String values:\n"); matte_heap_report_val(heap_track_info->values[MATTE_VALUE_TYPE_STRING]);
    printf("Object values:\n"); matte_heap_report_val(heap_track_info->values[MATTE_VALUE_TYPE_OBJECT]);
    printf("Type values:\n"); matte_heap_report_val(heap_track_info->values[MATTE_VALUE_TYPE_TYPE]);
    printf("|                                  |\n");
    printf("--=------------------------------=--\n\n");



}



#endif
