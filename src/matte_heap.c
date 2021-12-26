#include "matte_heap.h"
#include "matte_bin.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_string.h"
#include "matte_bytecode_stub.h"
#include "matte_vm.h"
#include "matte_heap_string.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef MATTE_DEBUG 
    #include <assert.h>
#endif

typedef struct {
    // name of the type
    matteValue_t name;
    
    // All the typecodes that this type can validly be used as in an 
    // isa comparison
    matteArray_t * isa;
} MatteTypeData;

struct matteHeap_t {    
    matteBin_t * sortedHeap;
    
    
    
    
    // objects with suspected ref count 0.
    // needs a double check before confirming.
    // matteObject_t *
    matteArray_t * toSweep;


    // Collection of objects that were detected to have been 
    // untraceable to a root object. This means that they are 
    // effectively untraceable and should be considered similarly 
    // to having a refcount of 0 (recycled)
    matteArray_t * toSweep_rootless;
    
    uint16_t gcLocked;
    uint8_t pathCheckedPool;
    uint8_t preserver;

    // pool for value types.
    uint32_t typepool;


    matteValue_t type_empty;
    matteValue_t type_boolean;
    matteValue_t type_number;
    matteValue_t type_string;
    matteValue_t type_object;
    matteValue_t type_function;
    matteValue_t type_type;
    matteValue_t type_any;
    
    // names for all types, on-demand
    // MatteTypeData
    matteArray_t * typecode2data;
    
    matteArray_t * routePather;
    matteTableIter_t * routeIter;
    // all objects contained in here are "verified as root-reachable"
    // for the reamainder of the root sweep.
    //matteTable_t * verifiedRoot;
    uint16_t verifiedCycle;
    uint16_t cooldown;
    int shutdown;

    matteVM_t * vm;

    matteStringHeap_t * stringHeap;
    
    
    // string value for "empty"
    matteValue_t specialString_empty;
    matteValue_t specialString_nothing;
    matteValue_t specialString_true;
    matteValue_t specialString_false;
    matteValue_t specialString_key;
    matteValue_t specialString_value;
    
    
    matteValue_t specialString_type_empty;
    matteValue_t specialString_type_boolean;
    matteValue_t specialString_type_number;
    matteValue_t specialString_type_string;
    matteValue_t specialString_type_object;
    matteValue_t specialString_type_function;
    matteValue_t specialString_type_type;
    
    matteValue_t specialString_get;
    matteValue_t specialString_set;
    matteValue_t specialString_bracketAccess;
    matteValue_t specialString_dotAccess;
    matteValue_t specialString_preserver;
    matteValue_t specialString_foreach;
    matteValue_t specialString_name;
    matteValue_t specialString_inherits;
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
    
    // if currently waiting to update its root status
    uint16_t toSweep;
    uint16_t dormant;

    uint32_t isRootless;
    // when refs are associated with other references,
    // a list of currently-active parents (owners of this ref) and 
    // children (the refs that consider this ref an owner) are 
    // maintained.
    //
    // Arrays of uint32_t, objectIDs to other objects.
    // references to non-objects are NOT maintained.
    //
    // Parents keep children references alive. Children "rely on" parents 
    // for their references
    matteArray_t * refChildren;
    matteArray_t * refParents;

    //matteTable_t * refChildren;
    //matteTable_t * refParents;

    // root refs keep children ref chains alive.
    int isRootRef;
    uint16_t verifiedRoot;
    int8_t isFunction;
    uint8_t routeChecked;
    // id within sorted heap.
    uint32_t heapID;
    uint8_t hasPreserver;
    
    union {
        struct {
            // keys -> matteValue_t
            matteTable_t * keyvalues_string;
            matteArray_t * keyvalues_number;
            matteTable_t * keyvalues_object;
            matteValue_t keyvalue_true;
            matteValue_t keyvalue_false;
            matteTable_t * keyvalues_types;

            matteValue_t attribSet;
            
            void (*nativeFinalizer)(void * objectUserdata, void * functionUserdata);
            void * nativeFinalizerData;

            
            // custom typecode.
            uint32_t typecode;
        } table;
        
        struct {
            // stub for functions. If null, is a normal object.
            matteBytecodeStub_t * stub;

            // call captured variables. Defined at function creation.
            matteArray_t * captures;

            // for type-strict functions, these are the function types 
            // asserted for args and return values.
            matteArray_t * types;

            // for function objects, a reference to the original 
            // creation context is kept. This is because referrables 
            // can track back to original creation contexts.
            matteValue_t origin;
            matteValue_t origin_referrable;        
        } function;
    };

} matteObject_t;




static matteValue_t * object_lookup(matteHeap_t * heap, matteObject_t * m, matteValue_t key) {
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return NULL;
      case MATTE_VALUE_TYPE_STRING: {
        return matte_table_find_by_uint(m->table.keyvalues_string, key.value.id);
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        if (key.value.number < 0) return NULL;
        uint32_t index = (uint32_t)key.value.number;
        if (index < matte_array_get_size(m->table.keyvalues_number)) {
            return &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        } else {
            return NULL;
        }   
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        if (key.value.boolean) {
            return &m->table.keyvalue_true;
        } else {
            return &m->table.keyvalue_false;
        }
      }
      
      case MATTE_VALUE_TYPE_FUNCTION:
      case MATTE_VALUE_TYPE_OBJECT: {           
        return matte_table_find_by_uint(m->table.keyvalues_object, key.value.id);
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        return matte_table_find_by_uint(m->table.keyvalues_types, key.value.id);
      }
    }
    return NULL;
}

typedef struct {
    uint32_t linkcount;
    uint32_t id;
} MatteHeapParentChildLink;
static void object_link_parent(matteObject_t * parent, matteObject_t * child) {
    uint32_t i;
    uint32_t lenP = matte_array_get_size(parent->refChildren);
    uint32_t lenC = matte_array_get_size(child->refParents);

    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->id == child->heapID) {
            iter->linkcount++;

            iter = matte_array_get_data(child->refParents);            
            for(i = 0; i < lenC; ++i, ++iter) {
                if (iter->id == parent->heapID) {
                    iter->linkcount++;
                    return;                
                }
            }

            return;            
        }
    }
    
    matte_array_set_size(parent->refChildren, lenP+1);
    iter = &matte_array_at(parent->refChildren, MatteHeapParentChildLink, lenP);
    iter->linkcount = 1;
    iter->id = child->heapID;

    matte_array_set_size(child->refParents, lenC+1);
    iter = &matte_array_at(child->refParents, MatteHeapParentChildLink, lenC);
    iter->linkcount = 1;
    iter->id = parent->heapID;



    //void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    //matte_table_insert_by_uint(child->refParents,   parent->heapID, p+1);
    //matte_table_insert_by_uint(parent->refChildren, child->heapID,  p+1);
}

static void object_unlink_parent(matteObject_t * parent, matteObject_t * child) {
    uint32_t i, n;
    uint32_t lenP = matte_array_get_size(parent->refChildren);
    uint32_t lenC = matte_array_get_size(child->refParents);

    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->id == child->heapID) {

            MatteHeapParentChildLink * iterO = matte_array_get_data(child->refParents);            
            for(n = 0; n < lenC; ++n, ++iterO) {
                if (iterO->id == parent->heapID) {
                                           
                    if (iter->linkcount == 1) {
                        matte_array_remove(parent->refChildren, i);
                        matte_array_remove(child->refParents, n);
                    } else {
                        iter->linkcount--;
                        iterO->linkcount--;
                    }
                    return;                
                }
            }

            return;            
        }
    }



    /*

    void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    if (p == (void*)0x1) {
        matte_table_remove_by_uint(child->refParents, parent->heapID);
        matte_table_remove_by_uint(parent->refChildren, child->heapID);
    } else {
        matte_table_insert_by_uint(child->refParents,   parent->heapID, p-1);
        matte_table_insert_by_uint(parent->refChildren, child->heapID,  p-1);
    }
    */
}

static void object_unlink_parent_child_only_all(matteObject_t * parent, matteObject_t * child) {
    uint32_t n;
    uint32_t lenC = matte_array_get_size(child->refParents);

    MatteHeapParentChildLink * iterO = matte_array_get_data(child->refParents);            
    for(n = 0; n < lenC; ++n, ++iterO) {
        if (iterO->id == parent->heapID) {
                                   
            matte_array_remove(child->refParents, n);
            return;                
        }
    }



    /*
    void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    if (p == (void*)0x1) {
        matte_table_remove_by_uint(child->refParents, parent->heapID);
    } else {
        matte_table_insert_by_uint(child->refParents,   parent->heapID, p-1);
    }
    */
}

static void object_unlink_parent_parent_only_all(matteObject_t * parent, matteObject_t * child) {
    uint32_t i;
    uint32_t lenP = matte_array_get_size(parent->refChildren);

    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->id == child->heapID) {                                           
            matte_array_remove(parent->refChildren, i);
            return;                
        }
    }
    /*
    void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    if (p == (void*)0x1) {
        matte_table_remove_by_uint(parent->refChildren, child->heapID);
    } else {
        matte_table_insert_by_uint(parent->refChildren, child->heapID,  p-1);
    }*/
}


static void object_link_parent_value(matteHeap_t * heap, matteObject_t * parent, const matteValue_t * child) {
    object_link_parent(parent,  matte_bin_fetch(heap->sortedHeap, child->value.id));
}

static void object_unlink_parent_value(matteHeap_t * heap, matteObject_t * parent, const matteValue_t * child) {
    object_unlink_parent(parent,  matte_bin_fetch(heap->sortedHeap, child->value.id));
}


static matteValue_t * object_put_prop(matteHeap_t * heap, matteObject_t * m, matteValue_t key, matteValue_t val) {
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_copy(heap, &out, val);

    // track reference
    if (val.binID == MATTE_VALUE_TYPE_OBJECT || val.binID == MATTE_VALUE_TYPE_FUNCTION) {
        object_link_parent_value(heap, m, &val);
    }

    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return NULL; // SHOULD NEVER ENTER. if it would, memory leak.
      case MATTE_VALUE_TYPE_STRING: {
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_string, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);
            }
            matte_heap_recycle(heap, *value);
            *value = out;
            return value;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert_by_uint(m->table.keyvalues_string, key.value.id, value);
            return value;
        }
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        if (key.value.number < 0) {
            matte_vm_raise_error_cstring(heap->vm, "Objects cannot have Number keys that are negative. Try using a string instead.");
            matte_heap_recycle(heap, out);
            return NULL;
        }
        uint32_t index = (uint32_t)key.value.number;
        if (index >= matte_array_get_size(m->table.keyvalues_number)) {
            uint32_t currentLen = matte_array_get_size(m->table.keyvalues_number);
            matte_array_set_size(m->table.keyvalues_number, index+1);
            uint32_t i;
            // new entries as empty
            for(i = currentLen; i < index+1; ++i) {
                matte_array_at(m->table.keyvalues_number, matteValue_t, i) = matte_heap_new_value(heap);
            }
        }

        matteValue_t * newV = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        if (newV->binID == MATTE_VALUE_TYPE_OBJECT || newV->binID == MATTE_VALUE_TYPE_FUNCTION) {
            object_unlink_parent_value(heap, m, newV);
        }

        matte_heap_recycle(heap, *newV);
        *newV = out;
        return newV;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {

        if (key.value.boolean) {
            if (m->table.keyvalue_true.binID == MATTE_VALUE_TYPE_OBJECT || m->table.keyvalue_true.binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, &m->table.keyvalue_true);
            }

            matte_heap_recycle(heap, m->table.keyvalue_true);
            m->table.keyvalue_true = out;            
            return &m->table.keyvalue_true;
        } else {
            if (m->table.keyvalue_false.binID == MATTE_VALUE_TYPE_OBJECT || m->table.keyvalue_false.binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, &m->table.keyvalue_false);
            }

            matte_heap_recycle(heap, m->table.keyvalue_false);
            m->table.keyvalue_false = out;            
            return &m->table.keyvalue_false;
        }
      }
      
      case MATTE_VALUE_TYPE_FUNCTION:           
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_object, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);
            }
            matte_heap_recycle(heap, *value);
            *value = out;
            return value;
        } else {
            object_link_parent_value(heap, m, &key);
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert_by_uint(m->table.keyvalues_object, key.value.id, value);
            return value;
        }
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_types, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);
            }
            matte_heap_recycle(heap, *value);
            *value = out;
            return value;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert_by_uint(m->table.keyvalues_types, key.value.id, value);
            return value;
        }
      }
    }
    return NULL;
}

// returns a type conversion operator if it exists
static matteValue_t object_get_conv_operator(matteHeap_t * heap, matteObject_t * m, uint32_t type) {
    matteValue_t out = matte_heap_new_value(heap);
    if (m->table.attribSet.binID == 0) return out;
    matteValue_t * operator = &m->table.attribSet;

    matteValue_t key;
    heap = heap;
    key.binID = MATTE_VALUE_TYPE_TYPE;
    key.value.id = type;
    // OK: empty doesnt allocate anything
    out = matte_value_object_access(heap, *operator, key, 0);
    return out;
}

// isDot is either 0 ([]) or 1 (.) 
// read if either 0 (write) or 1(read)
static matteValue_t object_get_access_operator(matteHeap_t * heap, matteObject_t * m, int isBracket, int read) {
    matteValue_t out = matte_heap_new_value(heap);
    if (m->table.attribSet.binID == 0) return out;


    
    matteValue_t set = matte_value_object_access(heap, m->table.attribSet, isBracket ? heap->specialString_bracketAccess : heap->specialString_dotAccess, 0);
    if (set.binID) {
        if (set.binID != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(heap->vm, "operator['[]'] and operator['.'] property must be an Object if it is set.");
        } else {
            out = matte_value_object_access(heap, set, read ? heap->specialString_get : heap->specialString_set, 0);
        }            
    }

    matte_heap_recycle(heap, set);            
    return out;
}



static void * create_object() {
    matteObject_t * out = calloc(1, sizeof(matteObject_t));
    out->isFunction = -1;
    //out->refChildren = matte_table_create_hash_pointer();
    //out->refParents = matte_table_create_hash_pointer();
    out->refChildren = matte_array_create(sizeof(MatteHeapParentChildLink));
    out->refParents = matte_array_create(sizeof(MatteHeapParentChildLink));

    return out;
}

static void destroy_object(void * d) {
    matteObject_t * out = d;

    if (out->isFunction == 1) {
        matte_array_destroy(out->function.captures);
    
    } else if (out->isFunction == 0){
        matteTableIter_t * iter = matte_table_iter_create();
        for(matte_table_iter_start(iter, out->table.keyvalues_string);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)
        ) {
            free(matte_table_iter_get_value(iter));
        }
        for(matte_table_iter_start(iter, out->table.keyvalues_types);
            !matte_table_iter_is_end(iter);
            matte_table_iter_proceed(iter)
        ) {
            free(matte_table_iter_get_value(iter));
        }

        matte_table_iter_destroy(iter);

        matte_table_destroy(out->table.keyvalues_string);
        matte_table_destroy(out->table.keyvalues_types);
        matte_table_destroy(out->table.keyvalues_object);
        matte_array_destroy(out->table.keyvalues_number);
    
    }


    //matte_table_destroy(out->refParents);
    //matte_table_destroy(out->refChildren);
    matte_array_destroy(out->refParents);
    matte_array_destroy(out->refChildren);
    free(out);
}





matteHeap_t * matte_heap_create(matteVM_t * vm) {
    matteHeap_t * out = calloc(1, sizeof(matteHeap_t));
    out->vm = vm;
    out->sortedHeap = matte_bin_create(create_object, destroy_object);
    out->typecode2data = matte_array_create(sizeof(MatteTypeData));
    out->routePather = matte_array_create(sizeof(void*));
    out->routeIter = matte_table_iter_create();
    //out->verifiedRoot = matte_table_create_hash_pointer();
    out->stringHeap = matte_string_heap_create();

    MatteTypeData dummyD = {};
    matte_array_push(out->typecode2data, dummyD);
    out->toSweep = matte_array_create(sizeof(matteObject_t *));
    out->toSweep_rootless = matte_array_create(sizeof(matteObject_t *));


    out->type_empty = matte_heap_new_value(out);
    out->type_boolean = matte_heap_new_value(out);
    out->type_number = matte_heap_new_value(out);
    out->type_string = matte_heap_new_value(out);
    out->type_object = matte_heap_new_value(out);
    out->type_function = matte_heap_new_value(out);
    out->type_type = matte_heap_new_value(out);
    out->type_any = matte_heap_new_value(out);

    matteValue_t dummy = {};
    matte_value_into_new_type(out, &out->type_empty, dummy, dummy);
    matte_value_into_new_type(out, &out->type_boolean, dummy, dummy);
    matte_value_into_new_type(out, &out->type_number, dummy, dummy);
    matte_value_into_new_type(out, &out->type_string, dummy, dummy);
    matte_value_into_new_type(out, &out->type_object, dummy, dummy);
    matte_value_into_new_type(out, &out->type_function, dummy, dummy);
    matte_value_into_new_type(out, &out->type_type, dummy, dummy);
    matte_value_into_new_type(out, &out->type_any, dummy, dummy);

    out->specialString_empty.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "empty");
    out->specialString_nothing.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "");
    out->specialString_true.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "true");
    out->specialString_false.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "false");

    out->specialString_type_boolean.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Boolean");
    out->specialString_type_empty.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Empty");
    out->specialString_type_number.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Number");
    out->specialString_type_string.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "String");
    out->specialString_type_object.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Object");
    out->specialString_type_type.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Type");
    out->specialString_type_function.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "Function");

    out->specialString_empty.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_nothing.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_true.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_false.binID = MATTE_VALUE_TYPE_STRING;



    out->specialString_type_boolean.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_empty.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_number.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_string.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_object.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_function.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_type.binID = MATTE_VALUE_TYPE_STRING;


    out->specialString_set.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "set");
    out->specialString_set.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_get.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "get");
    out->specialString_get.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_bracketAccess.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "[]");
    out->specialString_bracketAccess.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_dotAccess.value.id = matte_string_heap_internalize_cstring(out->stringHeap, ".");
    out->specialString_dotAccess.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_preserver.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "preserver");
    out->specialString_preserver.binID = MATTE_VALUE_TYPE_STRING;


    out->specialString_foreach.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "foreach");
    out->specialString_foreach.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_name.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "name");
    out->specialString_name.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_inherits.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "inherits");
    out->specialString_inherits.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_key.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "key");
    out->specialString_key.binID = MATTE_VALUE_TYPE_STRING;
    out->specialString_value.value.id = matte_string_heap_internalize_cstring(out->stringHeap, "value");
    out->specialString_value.binID = MATTE_VALUE_TYPE_STRING;


    return out;
    
}

void matte_heap_destroy(matteHeap_t * h) {

    h->shutdown = 1;
    while(matte_array_get_size(h->toSweep) || matte_array_get_size(h->toSweep_rootless)) {
        h->cooldown = 0;
        matte_heap_garbage_collect(h);
    }


    #ifdef MATTE_DEBUG__HEAP
    assert(matte_heap_report(h) == 0);
    #endif

    matte_array_destroy(h->toSweep);
    matte_array_destroy(h->toSweep_rootless);
    matte_bin_destroy(h->sortedHeap);
    uint32_t i;
    uint32_t len = matte_array_get_size(h->typecode2data);
    for(i = 0; i < len; ++i) {
        MatteTypeData d = matte_array_at(h->typecode2data, MatteTypeData, i);
        if (d.isa) {
            matte_array_destroy(d.isa);
        }
    }
    matte_array_destroy(h->typecode2data);
    matte_array_destroy(h->routePather);
    matte_table_iter_destroy(h->routeIter);
    //matte_table_destroy(h->verifiedRoot);
    matte_string_heap_destroy(h->stringHeap);
    free(h);
}


matteValue_t matte_heap_new_value_(matteHeap_t * h) {
    matteValue_t out;
    out.binID = 0;
    return out;
}

void matte_value_into_empty_(matteHeap_t * heap, matteValue_t * v) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_EMPTY;
}

int matte_value_is_empty(matteHeap_t * heap, matteValue_t v) {
    return v.binID == 0;
}


// Sets the type to a number with the given value
void matte_value_into_number_(matteHeap_t * heap, matteValue_t * v, double val) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_NUMBER;
    v->value.number = val;
}


static void isa_add(matteHeap_t * heap, matteArray_t * arr, matteValue_t * v) {
    matte_array_push(arr, v->value.id);
    // roll out all types
    MatteTypeData d = matte_array_at(heap->typecode2data, MatteTypeData, v->value.id);
    if (d.isa) {
        uint32_t i;
        uint32_t count = matte_array_get_size(d.isa);
        for(i = 0; i < count; ++i) {
            matte_array_push(arr, matte_array_at(d.isa, uint32_t, i));
        }
    }
}

static matteArray_t * type_array_to_isa(matteHeap_t * heap, matteValue_t val) {
    uint32_t count = matte_value_object_get_key_count(heap, val);
    uint32_t i;
    matteArray_t * array = matte_array_create(sizeof(uint32_t));

    if (count == 0) {
        matte_vm_raise_error_cstring(heap->vm, "'inherits' attribute cannot be empty.");
        return array;
    }

    for(i = 0; i < count; ++i) {
        matteValue_t * v = matte_value_object_array_at_unsafe(heap, val, i);
        if (v->binID != MATTE_VALUE_TYPE_TYPE) {
            matte_vm_raise_error_cstring(heap->vm, "'inherits' attribute must have Type values only.");
            return array;
        }
        isa_add(heap, array, v);
    }
    return array;
}

void matte_value_into_new_type_(matteHeap_t * heap, matteValue_t * v, matteValue_t name, matteValue_t inherits) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_TYPE;
    if (heap->typepool == 0xffffffff) {
        v->value.id = 0;
        matte_vm_raise_error_cstring(heap->vm, "Type count limit reached. No more types can be created.");
    } else {
        v->value.id = ++(heap->typepool);
    }
    MatteTypeData data = {};
    data.name = heap->specialString_empty;

    matteValue_t val = name;
    if (val.binID) {
        data.name = matte_value_as_string(heap, val);
        matte_heap_recycle(heap, val);
    } else {
        matteString_t * str = matte_string_create_from_c_str("<anonymous type %d>", v->value.id);
        data.name.value.id = matte_string_heap_internalize(heap->stringHeap, str);
        data.name.binID = MATTE_VALUE_TYPE_STRING;
        matte_string_destroy(str);        
    }


    val = inherits;
    if (val.binID) {
        if (val.binID != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(heap->vm, "'inherits' attribute must be an object.");
            matte_heap_recycle(heap, val);
            v->binID = 0;
            return;
        }            
        
        data.isa = type_array_to_isa(heap, val);
        matte_heap_recycle(heap, val);
    } 
                
    matte_array_push(heap->typecode2data, data);
}


// Sets the type to a number with the given value
void matte_value_into_boolean_(matteHeap_t * heap, matteValue_t * v, int val) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_BOOLEAN;
    v->value.boolean = val;
}

void matte_value_into_string_(matteHeap_t * heap, matteValue_t * v, const matteString_t * str) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_STRING;
    v->value.id = matte_string_heap_internalize(heap->stringHeap, str);
}

static void prep_object_as_table(matteObject_t * d) {
    if (d->isFunction == 1) {
        matte_array_destroy(d->function.captures);
    }
    
    if (d->isFunction != 0) {
        d->table.keyvalues_string = matte_table_create_hash_pointer();
        d->table.keyvalues_types  = matte_table_create_hash_pointer();
        d->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));
        d->table.keyvalues_object = matte_table_create_hash_pointer();
        d->table.keyvalue_true.binID = 0;
        d->table.keyvalue_false.binID = 0;       
        d->isFunction = 0;
    }
}

static void prep_object_as_function(matteObject_t * d) {
    if (d->isFunction == 0) {
        matte_table_destroy(d->table.keyvalues_string);    
        matte_table_destroy(d->table.keyvalues_types);    
        matte_table_destroy(d->table.keyvalues_object);    
        matte_array_destroy(d->table.keyvalues_number);    
    }
    if (d->isFunction != 1) {
        d->function.captures = matte_array_create(sizeof(CapturedReferrable_t));
        d->function.types = NULL;
        d->isFunction = 1;
    }
}

void matte_value_into_new_object_ref_(matteHeap_t * heap, matteValue_t * v) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->sortedHeap, &v->value.id);
    prep_object_as_table(d);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;
    d->dormant = 0;
}


void matte_value_into_new_object_ref_typed_(matteHeap_t * heap, matteValue_t * v, matteValue_t typeobj) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->sortedHeap, &v->value.id);
    prep_object_as_table(d);
    d->heapID = v->value.id;
    if (typeobj.binID != MATTE_VALUE_TYPE_TYPE) {        
        matteString_t * str = matte_string_create_from_c_str("Cannot instantiate object without a Type. (given value is of type %s)", matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, typeobj))));
        matte_vm_raise_error_string(heap->vm, str);
        matte_string_destroy(str);
        d->table.typecode = heap->type_object.value.id;
    } else {
        d->table.typecode = typeobj.value.id;
    }
    d->dormant = 0;
}


void matte_value_into_new_object_literal_ref_(matteHeap_t * heap, matteValue_t * v, const matteArray_t * arr) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->sortedHeap, &v->value.id);
    prep_object_as_table(d);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;

    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    
    for(i = 0; i < len; i+=2) {
        matteValue_t key   = matte_array_at(arr, matteValue_t, i);
        matteValue_t value = matte_array_at(arr, matteValue_t, i+1);
        
        object_put_prop(heap, d, key, value);
    }
    d->dormant = 0;


}


void matte_value_into_new_object_array_ref_(matteHeap_t * heap, matteValue_t * v, const matteArray_t * args) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->sortedHeap, &v->value.id);
    prep_object_as_table(d);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;

    uint32_t i;
    uint32_t len = matte_array_get_size(args);


    matte_array_set_size(d->table.keyvalues_number, len);
    for(i = 0; i < len; ++i) {
        matteValue_t val = matte_heap_new_value(heap);
        matte_value_into_copy(heap, &val, matte_array_at(args, matteValue_t, i));
        matte_array_at(d->table.keyvalues_number, matteValue_t, i) = val;
        if (val.binID == MATTE_VALUE_TYPE_OBJECT || val.binID == MATTE_VALUE_TYPE_FUNCTION) {
            object_link_parent_value(heap, d, &val);
        }
    }
    d->dormant = 0;

}

matteValue_t matte_value_frame_get_named_referrable(matteHeap_t * heap, matteVMStackFrame_t * vframe, matteValue_t name) {
    if (vframe->context.binID != MATTE_VALUE_TYPE_FUNCTION) return matte_heap_new_value(heap);
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, vframe->context.value.id);
    if (!m->function.stub) return matte_heap_new_value(heap);
    // claim captures immediately.
    uint32_t i;
    uint32_t len;



    // referrables come from a history of creation contexts.
    matteValue_t context = vframe->context;
    matteValue_t contextReferrable = vframe->referrable;

    matteValue_t out = matte_heap_new_value(heap);

    while(context.binID) {
        matteObject_t * origin = matte_bin_fetch(heap->sortedHeap, context.value.id);
        #if MATTE_DEBUG__HEAP
            printf("Looking in args..\n");
        #endif

        len = matte_bytecode_stub_arg_count(origin->function.stub);
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__HEAP
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_bytecode_stub_get_arg_name(origin->function.stub, i))),
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, name))
                );
            #endif
            matteValue_t argName = matte_bytecode_stub_get_arg_name(origin->function.stub, i);
            if (argName.value.id == name.value.id) {
                matte_value_into_copy(heap, &out, *matte_value_object_array_at_unsafe(heap, contextReferrable, 1+i));
                return out;
            }
        }
        
        len = matte_bytecode_stub_local_count(origin->function.stub);
        #if MATTE_DEBUG__HEAP
            printf("Looking in locals..\n");
        #endif
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__HEAP
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_bytecode_stub_get_local_name(origin->function.stub, i))),
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, name))
                );
            #endif
            if (matte_bytecode_stub_get_local_name(origin->function.stub, i).value.id == name.value.id) {
                matte_value_into_copy(heap, &out, *matte_value_object_array_at_unsafe(heap, contextReferrable, 1+i+matte_bytecode_stub_arg_count(origin->function.stub)));
                return out;
            }
        }

        context = origin->function.origin;
        contextReferrable = origin->function.origin_referrable;
    }

    matte_vm_raise_error_cstring(heap->vm, "Could not find named referrable!");
    return out;
}


void matte_value_into_new_typed_function_ref_(matteHeap_t * heap, matteValue_t * v, matteBytecodeStub_t * stub, const matteArray_t * args) {
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
            
            matte_vm_raise_error_string(heap->vm, str);
            matte_string_destroy(str);
            return;
        }
    }
    
    matte_value_into_new_function_ref(heap, v, stub);
    matteObject_t * d = matte_bin_fetch(heap->sortedHeap, v->value.id);
    d->function.types = matte_array_clone(args);
}

void matte_value_into_new_function_ref_(matteHeap_t * heap, matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_FUNCTION;
    matteObject_t * d = matte_bin_add(heap->sortedHeap, &v->value.id);
    prep_object_as_function(d);
    d->heapID = v->value.id;
    d->function.stub = stub;
    d->dormant = 0;



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
    if (matte_vm_get_stackframe_size(heap->vm)) {
        frame = matte_vm_get_stackframe(heap->vm, 0);

        matte_value_into_copy(heap, &d->function.origin, frame.context);
        matte_value_into_copy(heap, &d->function.origin_referrable, frame.referrable);

        object_link_parent_value(heap, d, &d->function.origin);
        object_link_parent_value(heap, d, &d->function.origin_referrable);

    }
    // TODO: can we speed this up?
    if (!len) return;


    // referrables come from a history of creation contexts.
    for(i = 0; i < len; ++i) {
        matteValue_t context = frame.context;
        matteValue_t contextReferrable = frame.referrable;

        while(context.binID) {
            matteObject_t * origin = matte_bin_fetch(heap->sortedHeap, context.value.id);
            if (matte_bytecode_stub_get_id(origin->function.stub) == capturesRaw[i].stubID) {
                CapturedReferrable_t ref;
                ref.referrableSrc = matte_heap_new_value(heap);
                matte_value_into_copy(heap, &ref.referrableSrc, contextReferrable);
                if (contextReferrable.binID == MATTE_VALUE_TYPE_OBJECT || contextReferrable.binID == MATTE_VALUE_TYPE_FUNCTION)
                    object_link_parent_value(heap, d, &contextReferrable);

                ref.index = capturesRaw[i].referrable;

                matte_array_push(d->function.captures, ref);
                break;
            }
            context = origin->function.origin;
            contextReferrable = origin->function.origin_referrable;
        }

        if (!context.binID) {
            matte_vm_raise_error_cstring(heap->vm, "Could not find captured variable!");
            CapturedReferrable_t ref;
            ref.referrableSrc = matte_heap_new_value(heap);
            ref.index = 0;
            matte_array_push(d->function.captures, ref);
        }
    }
}


matteValue_t * matte_value_object_array_at_unsafe(matteHeap_t * heap, matteValue_t v, uint32_t index) {
    matteObject_t * d = matte_bin_fetch(heap->sortedHeap, v.value.id);
    return &matte_array_at(d->table.keyvalues_number, matteValue_t, index);
}


const matteString_t * matte_value_string_get_string_unsafe(matteHeap_t * heap, matteValue_t v) {
    #ifdef MATTE_DEBUG 
    assert(v.binID == MATTE_VALUE_TYPE_STRING);
    #endif
    return matte_string_heap_find(heap->stringHeap, v.value.id);
}


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteHeap_t * heap, matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        return m->function.stub;
    }
    return NULL;
}

matteValue_t * matte_value_get_captured_value(matteHeap_t * heap, matteValue_t v, uint32_t index) {
    if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        if (index >= matte_array_get_size(m->function.captures)) return NULL;
        CapturedReferrable_t referrable = matte_array_at(m->function.captures, CapturedReferrable_t, index);
        return matte_value_object_array_at_unsafe(heap, referrable.referrableSrc, referrable.index);
    }
    return NULL;
}

void matte_value_set_captured_value(matteHeap_t * heap, matteValue_t v, uint32_t index, matteValue_t val) {
    if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        if (index >= matte_array_get_size(m->function.captures)) return;

        CapturedReferrable_t referrable = matte_array_at(m->function.captures, CapturedReferrable_t, index);
        matteValue_t vkey = matte_heap_new_value(heap);
        matte_value_into_number(heap, &vkey, referrable.index);
        matte_value_object_set(heap, referrable.referrableSrc, vkey, val, 1);
        matte_heap_recycle(heap, vkey);        
    }
}

#ifdef MATTE_DEBUG
void matte_heap_object_print_children(matteHeap_t * h, uint32_t id) {
    matteObject_t * m =  matte_bin_fetch(h->sortedHeap, id);
    if (!m) {
        printf("{cannot print children: is NULL?}\n");
        return;
    }

    int n;
    /*
    matteTableIter_t * iter = matte_table_iter_create();
    for(
        matte_table_iter_start(iter, m->refChildren);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)
    ) {
        uint32_t nc = matte_table_iter_get_key_uint(iter);
    */
    uint32_t i;
    uint32_t len = matte_array_get_size(m->refChildren);
    for(i = 0; i < len; ++i) {
        uint32_t nc = matte_array_at(m->refChildren, MatteHeapParentChildLink, i).id;
        printf("  {%d}", nc);
        printf("\n");

    }
    //matte_table_iter_destroy(iter);
}

void matte_heap_object_print_parents(matteHeap_t * h, uint32_t id) {
    matteObject_t * m =  matte_bin_fetch(h->sortedHeap, id);
    if (!m) {
        printf("{cannot print parents: is NULL?}\n");
        return;
    }
    /*
    matteTableIter_t * iter = matte_table_iter_create();
    int n;
    for(
        matte_table_iter_start(iter, m->refParents);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)
    ) {
    */
    uint32_t i;
    uint32_t len = matte_array_get_size(m->refParents);
    for(i = 0; i < len; ++i) {

        uint32_t nc = matte_array_at(m->refParents, MatteHeapParentChildLink, i).id;
        printf("  {%d}", nc);
        printf("\n");

    }
    //matte_table_iter_destroy(iter);
}


void matte_value_print(matteHeap_t * heap, matteValue_t v) {
    printf(    "Matte HEAP Object:\n");
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        printf("  Heap Type: EMPTY\n");
        break;
      case MATTE_VALUE_TYPE_BOOLEAN: 
        printf("  Heap Type: BOOLEAN\n");
        printf("  Value:     %s\n", matte_value_as_boolean(heap, v) ? "true" : "false");
        break;
      case MATTE_VALUE_TYPE_NUMBER: 
        printf("  Heap Type: NUMBER\n");
        printf("  Value:     %f\n", matte_value_as_number(heap, v));
        break;
      case MATTE_VALUE_TYPE_STRING: 
        printf("  Heap Type: STRING\n");
        printf("  String ID:   %d\n", v.value.id);
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, v)));
        break;
      case MATTE_VALUE_TYPE_TYPE: 
        printf("  Heap Type: TYPE\n");
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, v))));
        break;
      case MATTE_VALUE_TYPE_FUNCTION: 
        printf("  [FUNCTION]\n");
    
      case MATTE_VALUE_TYPE_OBJECT: {
        printf("  Heap Type: OBJECT\n");
        printf("  Heap ID:   %d\n", v.value.id);
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);            
        if (!m) {
            printf("  (WARNING: this refers to an object fully recycled.)\n");
            fflush(stdout);
            return;
        }
        if (m->dormant) {
            printf("  (WARNING: this object is currently dormant.)\n") ;          
        }
        printf("  RootLock:  %d\n", m->isRootRef);
        printf("  ToSweep?   %s\n", m->toSweep ? "true" : "false");
        printf("  Parents?   %s\n", !matte_array_get_size(m->refParents) ? "false" : "true");
        matte_heap_object_print_parents(heap, m->heapID);
        printf("  Children?  %s\n", !matte_array_get_size(m->refChildren) ? "false" : "true");
        matte_heap_object_print_children(heap, m->heapID);
        /*{
            printf("  String Values:\n");
            matteObject_t * m = matte_bin_fetch(heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.value.id);            
            matteTableIter_t * iter = matte_table_iter_create();
            for( matte_table_iter_start(iter, m->table.keyvalues_string);
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


void matte_value_print_from_id(matteHeap_t * heap, uint32_t bin, uint32_t v) {
    matteValue_t val;
    val.binID = bin;
    val.value.id = v;

    matte_value_print(heap, val);
}
#endif

void matte_value_object_set_native_finalizer(matteHeap_t * heap, matteValue_t v, void (*fb)(void * objectUserdata, void * functionUserdata), void * functionUserdata) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(heap->vm, "Tried to set native finalizer on a non-object value.");
        return;
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    m->table.nativeFinalizer = fb;
    m->table.nativeFinalizerData = functionUserdata;        
}


void matte_value_object_set_userdata(matteHeap_t * heap, matteValue_t v, void * userData) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        m->userdata = userData;
    }
}

void * matte_value_object_get_userdata(matteHeap_t * heap, matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        return m->userdata;
    }
    return NULL;
}



double matte_value_as_number(matteHeap_t * heap, matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        matte_vm_raise_error_cstring(heap->vm, "Cannot convert empty into a number.");
        return 0;
      case MATTE_VALUE_TYPE_TYPE: 
        matte_vm_raise_error_cstring(heap->vm, "Cannot convert type value into a number.");
        return 0;

      case MATTE_VALUE_TYPE_NUMBER: {
        return v.value.number;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        return v.value.boolean;
      }
      case MATTE_VALUE_TYPE_STRING: {
        matte_vm_raise_error_cstring(heap->vm, "Cannot convert string value into a number.");
        return 0;
      }
       case MATTE_VALUE_TYPE_FUNCTION: 
        matte_vm_raise_error_cstring(heap->vm, "Cannot convert function value into a number.");
        return 0;

      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        matteValue_t operator = object_get_conv_operator(heap, m, heap->type_number.value.id);
        if (operator.binID) {            
            double num = matte_value_as_number(heap, matte_vm_call(heap->vm, operator, matte_array_empty(), matte_array_empty(), NULL));
            matte_heap_recycle(heap, operator);
            return num;
        } else {
            matte_vm_raise_error_cstring(heap->vm, "Cannot convert object into a number. No 'Number' property in 'operator' object.");
        }
      }
    }
    return 0;
}

matteValue_t matte_value_as_string(matteHeap_t * heap, matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        //matte_vm_raise_error_cstring(heap->vm, "Cannot convert empty into a string.");
        ///return 0;
        return heap->specialString_empty;

      case MATTE_VALUE_TYPE_TYPE: { 
        return matte_value_type_name(heap, v);
      }

      // slow.. make fast please!
      case MATTE_VALUE_TYPE_NUMBER: {        
        matteString_t * str = matte_string_create_from_c_str("%g", v.value.number);
        matteValue_t out;
        out.binID = MATTE_VALUE_TYPE_STRING;
        out.value.id = matte_string_heap_internalize(heap->stringHeap, str);
        matte_string_destroy(str);        
        return out;        
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        return v.value.boolean ? heap->specialString_true : heap->specialString_false;
      }
      case MATTE_VALUE_TYPE_STRING: {
        return v;
      }
      
      case MATTE_VALUE_TYPE_FUNCTION: { 
        matte_vm_raise_error_cstring(heap->vm, "Cannot convert function into a string.");
        return heap->specialString_empty;
      }


      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        matteValue_t operator = object_get_conv_operator(heap, m, heap->type_string.value.id);
        if (operator.binID) {
            matteValue_t result = matte_vm_call(heap->vm, operator, matte_array_empty(), matte_array_empty(), NULL);
            matte_heap_recycle(heap, operator);
            matteValue_t f = matte_value_as_string(heap, result);
            matte_heap_recycle(heap, result);
            return f;
        } else {
            matte_vm_raise_error_cstring(heap->vm, "Object has no valid conversion to string.");
        }
      }
    }
    return heap->specialString_empty;
}


matteValue_t matte_value_to_type(matteHeap_t * heap, matteValue_t v, matteValue_t t) {
    matteValue_t out = matte_heap_new_value(heap);
    switch(t.value.id) {
      case 1: { // empty
        if (v.binID != MATTE_VALUE_TYPE_EMPTY) {
            matte_vm_raise_error_cstring(heap->vm, "It is an error to convert any non-empty value to the Empty type.");
        } else {
            matte_value_into_copy(heap, &out, v);
        }

        break;
      }

      case 7: {
        if (v.binID != MATTE_VALUE_TYPE_TYPE) {
            matte_vm_raise_error_cstring(heap->vm, "It is an error to convert any non-Type value to a Type.");
        } else {
            matte_value_into_copy(heap, &out, v);
        }
        break;
      }

      
      case 2: {
        matte_value_into_boolean(heap, &out, matte_value_as_boolean(heap, v));
        break;
      }
      case 3: {
        matte_value_into_number(heap, &out, matte_value_as_number(heap, v));
        break;
      }


      case 4: {
        out = matte_value_as_string(heap, v);
        break;
      }

      case 6: {
        if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
            matte_value_into_copy(heap, &out, v);
            break;
        } else {
            matte_vm_raise_error_cstring(heap->vm, "Cannot convert value to Function type.");        
        }
      }


      case 5: {
        if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
            matte_value_into_copy(heap, &out, v);
            break;
        } else {
            matte_vm_raise_error_cstring(heap->vm, "Cannot convert value to Object type.");        
        }
      
      }
      
      default:;// TODO: custom types.
      
      
    
    }
    return out;
}

int matte_value_as_boolean(matteHeap_t * heap, matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_TYPE: {
        return 1;
      }
      case MATTE_VALUE_TYPE_NUMBER: {
        return v.value.number != 0.0;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        return v.value.boolean;
      }
      case MATTE_VALUE_TYPE_STRING: {
        // non-empty value
        return 1;
      }
      case MATTE_VALUE_TYPE_FUNCTION: {
        // non-empty
        return 1;              
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        matteValue_t operator = object_get_conv_operator(heap, m, heap->type_boolean.value.id);
        if (operator.binID) {
            matteValue_t r = matte_vm_call(heap->vm, operator, matte_array_empty(), matte_array_empty(), NULL);
            int out = matte_value_as_boolean(heap, r);
            matte_heap_recycle(heap, operator);
            matte_heap_recycle(heap, r);
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
int matte_value_is_callable(matteHeap_t * heap, matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_TYPE) return 1;
    if (v.binID != MATTE_VALUE_TYPE_FUNCTION) return 0;
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    return 1 + (m->function.types != NULL); 
}

int matte_value_object_function_pre_typecheck_unsafe(matteHeap_t * heap, matteValue_t v, const matteArray_t * arr) {
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);

    #ifdef MATTE_DEBUG__HEAP
        assert(len == matte_array_get_size(m->function.types) - 1);    
    #endif    

    for(i = 0; i < len; ++i) {
        if (!matte_value_isa(
            heap, 
            matte_array_at(arr, matteValue_t, i),
            matte_array_at(m->function.types, matteValue_t, i)
        )) {
            matteString_t * err = matte_string_create_from_c_str(
                "Argument %d (type: %s) is not of required type %s",
                i+1,
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, matte_array_at(arr, matteValue_t, i))))),
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_array_at(m->function.types, matteValue_t, i))))
            );
            matte_vm_raise_error_string(heap->vm, err);
            matte_string_destroy(err);
            return 0;
        }
    }
    
    return 1;
}

void matte_value_object_function_post_typecheck_unsafe(matteHeap_t * heap, matteValue_t v, matteValue_t ret) {
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    if (!matte_value_isa(heap, ret, matte_array_at(m->function.types, matteValue_t, matte_array_get_size(m->function.types) - 1))) {
        matteString_t * err = matte_string_create_from_c_str(
            "Return value (type: %s) is not of required type %s.",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, ret)))),
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_array_at(m->function.types, matteValue_t, matte_array_get_size(m->function.types) - 1))))

        );
        matte_vm_raise_error_string(heap->vm, err);
        matte_string_destroy(err);
    }  
}


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteHeap_t * heap, matteValue_t v, matteValue_t key, int isBracketAccess) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, v))))
        );
        matte_vm_raise_error_string(heap->vm, err);
        return matte_heap_new_value(heap);
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteValue_t accessor = object_get_access_operator(heap, m, isBracketAccess, 1);
    if (accessor.binID) {
        matteValue_t args[1] = {
            key
        };
        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 1);
        matteArray_t arrName = MATTE_ARRAY_CAST(&heap->specialString_key, matteValue_t, 1);

        matteValue_t a = matte_vm_call(heap->vm, accessor, &arr, &arrName, NULL);
        matte_heap_recycle(heap, accessor);
        return a;
    } else {
        matteValue_t vv = matte_heap_new_value(heap);
        matteValue_t * vvp = object_lookup(heap, m, key);
        if (vvp) {
            matte_value_into_copy(heap, &vv, *vvp);
        }
        return vv;
    }
}


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t * matte_value_object_access_direct(matteHeap_t * heap, matteValue_t v, matteValue_t key, int isBracketAccess) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, v))))
        );
        matte_vm_raise_error_string(heap->vm, err);
        return NULL; //matte_heap_new_value(heap);
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteValue_t accessor = object_get_access_operator(heap, m, isBracketAccess, 1);
    if (accessor.binID) {
        return NULL;
    } else {
        return object_lookup(heap, m, key);
    }
}



matteValue_t matte_value_object_access_string(matteHeap_t * heap, matteValue_t v, const matteString_t * key) {
    matteValue_t keyO = matte_heap_new_value(heap);
    matte_value_into_string(heap, &keyO, key);
    matteValue_t result = matte_value_object_access(heap, v, keyO, 0);
    matte_heap_recycle(heap, keyO);
    return result;
}

matteValue_t matte_value_object_access_index(matteHeap_t * heap, matteValue_t v, uint32_t index) {
    matteValue_t keyO = matte_heap_new_value(heap);
    matte_value_into_number(heap, &keyO, index);
    matteValue_t result = matte_value_object_access(heap, v, keyO, 1);
    matte_heap_recycle(heap, keyO);
    return result;
}


void matte_value_object_push_lock_(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT && v.binID != MATTE_VALUE_TYPE_FUNCTION) return;
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    m->isRootRef++;
}

void matte_value_object_pop_lock_(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT && v.binID != MATTE_VALUE_TYPE_FUNCTION) return;
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    if (m->isRootRef)
        m->isRootRef--;
    if (!m->isRootRef) 
        matte_heap_recycle(heap, v);
}



matteValue_t matte_value_object_keys(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(heap);
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->table.keyvalues_number);
    if (len) {
        for(i = 0; i < len; ++i) {
            matteValue_t key = matte_heap_new_value(heap);
            matte_value_into_number(heap, &key, i);
            matte_array_push(keys, key);
        }
    }

    // then string
    if (!matte_table_is_empty(m->table.keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key = matte_heap_new_value(heap);
            key.binID = MATTE_VALUE_TYPE_STRING;
            key.value.id = matte_table_iter_get_key_uint(iter);
            matte_array_push(keys, key);                
        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->table.keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key;
            matteObject_t * current = matte_bin_fetch(heap->sortedHeap, matte_table_iter_get_key_uint(iter));
            key.binID = current->isFunction ? MATTE_VALUE_TYPE_FUNCTION : MATTE_VALUE_TYPE_OBJECT;
            key.value.id = current->heapID;
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_in(heap, key, "object.keys()", 0);
            #endif
            matte_array_push(keys, key);    
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->table.keyvalue_true.binID) {
        matteValue_t key = matte_heap_new_value(heap);
        matte_value_into_boolean(heap, &key, 1);
        matte_array_push(keys, key);
    }
    // false
    if (m->table.keyvalue_false.binID) {
        matteValue_t key = matte_heap_new_value(heap);
        matte_value_into_boolean(heap, &key, 0);
        matte_array_push(keys, key);
    }
    
    // then types
    if (!matte_table_is_empty(m->table.keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key;
            heap = heap;
            key.binID = MATTE_VALUE_TYPE_TYPE;
            key.value.id = matte_table_iter_get_key_uint(iter);

            matte_array_push(keys, key);                
        }
        matte_table_iter_destroy(iter);
    }
    
    val = matte_heap_new_value(heap);
    matte_value_into_new_object_array_ref(heap, &val, keys);
    len = matte_array_get_size(keys);
    for(i = 0; i < len; ++i) {
        matte_heap_recycle(heap, matte_array_at(keys, matteValue_t, i));
    }
    matte_array_destroy(keys);
    return val;
}

matteValue_t matte_value_object_values(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(heap);
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteArray_t * vals = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->table.keyvalues_number);
    if (len) {
        for(i = 0; i < len; ++i) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, matte_array_at(m->table.keyvalues_number, matteValue_t, i));
            matte_array_push(vals, val);
        }
    }

    // then string
    if (!matte_table_is_empty(m->table.keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);                
        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->table.keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);    
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->table.keyvalue_true.binID) {
        val = matte_heap_new_value(heap);
        matte_value_into_copy(heap, &val, m->table.keyvalue_true);
        matte_array_push(vals, val);
    }
    // false
    if (m->table.keyvalue_false.binID) {
        val = matte_heap_new_value(heap);
        matte_value_into_copy(heap, &val, m->table.keyvalue_false);
        matte_array_push(vals, val);
    }
    
    // then types
    if (!matte_table_is_empty(m->table.keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);                
        }
        matte_table_iter_destroy(iter);
    }

    val = matte_heap_new_value(heap);
    matte_value_into_new_object_array_ref(heap, &val, vals);
    len = matte_array_get_size(vals);
    for(i = 0; i < len; ++i) {
        matte_heap_recycle(heap, matte_array_at(vals, matteValue_t, i));
    }
    matte_array_destroy(vals);
    return val;
}


uint32_t matte_value_object_get_key_count(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return 0;
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    uint32_t total = 
        matte_array_get_size(m->table.keyvalues_number) +
        (m->table.keyvalue_true.binID?1:0) +
        (m->table.keyvalue_false.binID?1:0);

    // maybe maintain through get/set/remove prop?
    if (!matte_table_is_empty(m->table.keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }


    if (!matte_table_is_empty(m->table.keyvalues_object)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }
    
    if (!matte_table_is_empty(m->table.keyvalues_types)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }
    return total;    
}

void matte_value_object_remove_key_string(matteHeap_t * heap, matteValue_t v, const matteString_t * kstr) {
    matteValue_t k = matte_heap_new_value(heap);
    matte_value_into_string(heap, &k, kstr);
    matte_value_object_remove_key(heap, v, k);
    matte_heap_recycle(heap, k);
}

void matte_value_object_remove_key(matteHeap_t * heap, matteValue_t v, matteValue_t key) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return;
      case MATTE_VALUE_TYPE_STRING: {
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_string, key.value.id);
        if (value) {
            matte_table_remove_by_uint(m->table.keyvalues_string, key.value.id);
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);                
            }
            matte_heap_recycle(heap, *value);
            free(value);
        }
        return;
      }

      case MATTE_VALUE_TYPE_TYPE: {
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_types, key.value.id);
        if (value) {
            matte_table_remove_by_uint(m->table.keyvalues_types, key.value.id);
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);                
            }
            matte_heap_recycle(heap, *value);
            free(value);
        }
        return;
      }

      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        uint32_t index = (uint32_t) key.value.number;
        if (index >= matte_array_get_size(m->table.keyvalues_number)) {
            return;
        }
        matteValue_t * value = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
            object_unlink_parent_value(heap, m, value);                
        }

        matte_heap_recycle(heap, *value);
        matte_array_remove(m->table.keyvalues_number, index);
        return;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {

        if (key.value.boolean) {
            if (m->table.keyvalue_true.binID == MATTE_VALUE_TYPE_OBJECT || m->table.keyvalue_true.binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, &m->table.keyvalue_true);                
            }
            matte_heap_recycle(heap, m->table.keyvalue_true);
            m->table.keyvalue_true = matte_heap_new_value(heap);            
        } else {
            if (m->table.keyvalue_false.binID == MATTE_VALUE_TYPE_OBJECT || m->table.keyvalue_false.binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, &m->table.keyvalue_false);                
            }
            matte_heap_recycle(heap, m->table.keyvalue_false);
            m->table.keyvalue_false = matte_heap_new_value(heap);               
        }
        return;
      }
      
      case MATTE_VALUE_TYPE_FUNCTION:          
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_object, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);                
            }
            object_link_parent_value(heap, m, &key);
            matte_heap_recycle(heap, *value);
            matte_table_remove_by_uint(m->table.keyvalues_object, key.value.id);
        }
        return;
      }
    }

}

void matte_value_object_foreach(matteHeap_t * heap, matteValue_t v, matteValue_t func) {

    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);

    // foreach operator
    if (m->table.attribSet.binID) {
        matteValue_t set = matte_value_object_access(heap, m->table.attribSet, heap->specialString_foreach, 0);

        if (set.binID) {
            v = matte_vm_call(heap->vm, set, matte_array_empty(), matte_array_empty(), NULL);
            if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
                matte_vm_raise_error_string(heap->vm, MATTE_VM_STR_CAST(heap->vm, "foreach attribute MUST return a function."));
                return;
            } else {
                m = matte_bin_fetch(heap->sortedHeap, v.value.id);
            }
        }
    }
    matte_value_object_push_lock(heap, v);

    matteValue_t args[] = {
        matte_heap_new_value(heap),
        matte_heap_new_value(heap)
    };
    matteValue_t argNames[2] = {
        heap->specialString_key,
        heap->specialString_value,
    };

    matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(heap, func);
    // dynamic name binding for foreach
    if (stub && matte_bytecode_stub_arg_count(stub) >= 2) {
        argNames[0] = matte_bytecode_stub_get_arg_name(stub, 0);
        argNames[1] = matte_bytecode_stub_get_arg_name(stub, 1);
    }

    matteArray_t argNames_array = MATTE_ARRAY_CAST(argNames, matteValue_t, 2);


    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->table.keyvalues_number);
    if (len) {
        matte_value_into_number(heap, &args[0], 0);
        double * quickI = &args[0].value.number;

        for(i = 0; i < len; ++i) {
            args[1] = matte_array_at(m->table.keyvalues_number, matteValue_t, i);
            *quickI = i;
            matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
            matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);
            matte_heap_recycle(heap, r);
        }
        matte_heap_recycle(heap, args[0]);
        args[0].binID = 0;
    }

    // then string
    if (!matte_table_is_empty(m->table.keyvalues_string)) {
        matteString_t * e = matte_string_create();
        matte_value_into_string(heap, &args[0], e);
        matte_string_destroy(e);
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
            matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);
            matte_heap_recycle(heap, r);
        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (!matte_table_is_empty(m->table.keyvalues_object)) {
        args[0].binID = MATTE_VALUE_TYPE_FUNCTION;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_object);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
            matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);
            matte_heap_recycle(heap, r);
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->table.keyvalue_true.binID) {
        matte_value_into_boolean(heap, &args[0], 1);
        args[1] = m->table.keyvalue_true;
        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);        
        matte_heap_recycle(heap, args[0]);
        args[0].binID = 0;
        matte_heap_recycle(heap, r);
    }
    // false
    if (m->table.keyvalue_false.binID) {
        matte_value_into_boolean(heap, &args[0], 1);
        args[1] = m->table.keyvalue_false;
        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);        
        matte_heap_recycle(heap, r);
        matte_heap_recycle(heap, args[0]);
        args[0].binID = 0;
    }

    if (!matte_table_is_empty(m->table.keyvalues_types)) {
        matte_heap_recycle(heap, args[0]);
        args[0].binID = MATTE_VALUE_TYPE_TYPE;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
            matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);
            matte_heap_recycle(heap, r);
        }
        matte_heap_recycle(heap, args[0]);
        args[0].binID = 0;
        matte_table_iter_destroy(iter);
    }
    matte_value_object_pop_lock(heap, v);

}

matteValue_t matte_value_subset(matteHeap_t * heap, matteValue_t v, uint32_t from, uint32_t to) {
    matteValue_t out = matte_heap_new_value(heap);
    if (from > to) return out;

    switch(v.binID) {
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);


        uint32_t curlen = matte_array_get_size(m->table.keyvalues_number);
        if (from >= curlen || to >= curlen) return out;


        matteArray_t arr = MATTE_ARRAY_CAST(
            ((matteValue_t *)matte_array_get_data(m->table.keyvalues_number))+from,
            matteValue_t,
            (to-from)+1
        );
        matte_value_into_new_object_array_ref(
            heap,
            &out,
            &arr
        );

        break;
      }


      case MATTE_VALUE_TYPE_STRING: {
        const matteString_t * str = matte_string_heap_find(heap->stringHeap, v.value.id);
        uint32_t curlen = matte_string_get_length(str);
        if (from >= curlen || to >= curlen) return out;

        matte_value_into_string(heap, &out, matte_string_get_substr(str, from, to));
      }


    }
    return out;
}

matteValue_t matte_value_object_array_to_string_unsafe(matteHeap_t * heap, matteValue_t v) {
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);

    matteString_t * str = matte_string_create();
    uint32_t len = matte_array_get_size(m->table.keyvalues_number);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        matteValue_t * vn = &matte_array_at(m->table.keyvalues_number, matteValue_t, i);        
        if (vn->binID == MATTE_VALUE_TYPE_NUMBER) {
            matte_string_append_char(
                str,
                (uint32_t)vn->value.number
            );
        }
    }
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, str);
    matte_string_destroy(str);
    return out;
}


void matte_value_object_set_attributes(matteHeap_t * heap, matteValue_t v, matteValue_t opObject) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(heap->vm, "Cannot assign attributes set to something that isnt an object.");
        return;
    }
    
    if (opObject.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(heap->vm, "Cannot assign attributes set that isn't an object.");
        return;
    }
    
    if (opObject.value.id == v.value.id) {
        matte_vm_raise_error_cstring(heap->vm, "Cannot assign attributes set as its own object.");
        return;    
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    if (m->table.attribSet.binID) {
        object_unlink_parent_value(heap, m, &m->table.attribSet);
        matte_heap_recycle(heap, m->table.attribSet);
    }

    // can only use the preserver flag if we are NOT in the shutdown state
    if (matte_value_object_access_direct(heap, opObject, heap->specialString_preserver, 1) && !heap->shutdown) {   
        m->hasPreserver = 1;    
    } else {
        m->hasPreserver = 0;    
    }
    
    matte_value_into_copy(heap, &m->table.attribSet, opObject);
    object_link_parent_value(heap, m, &m->table.attribSet);
}

const matteValue_t * matte_value_object_get_attributes_unsafe(matteHeap_t * heap, matteValue_t v) {
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    if (m->table.attribSet.binID) return &m->table.attribSet;
    return NULL;
}


const matteValue_t * matte_value_object_set(matteHeap_t * heap, matteValue_t v, matteValue_t key, matteValue_t value, int isBracket) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(heap->vm, "Cannot set property on something that isnt an object.");
        return NULL;
    }
    if (key.binID == 0) {
        matte_vm_raise_error_cstring(heap->vm, "Cannot set property with an empty key");
        return NULL;
    }
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteValue_t assigner = object_get_access_operator(heap, m, isBracket, 0);
    if (assigner.binID) {
        matteValue_t args[2] = {
            key, value
        };
        matteValue_t argNames[] = {
            heap->specialString_key,
            heap->specialString_value,
        };
        matteArray_t argNames_array = MATTE_ARRAY_CAST(argNames, matteValue_t, 2);

        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(heap->vm, assigner, &arr, &argNames_array, NULL);
        matte_heap_recycle(heap, assigner);
        if (r.binID == MATTE_VALUE_TYPE_BOOLEAN && !matte_value_as_boolean(heap, r)) {
            matte_heap_recycle(heap, r);
            return NULL;
        } else {
            matte_heap_recycle(heap, r);
            return object_put_prop(heap, m, key, value); 
        }
    } else {
        return object_put_prop(heap, m, key, value);
    }
}

void matte_value_object_set_index_unsafe(matteHeap_t * heap, matteValue_t v, uint32_t index, matteValue_t val) {
    matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
    matteValue_t * newV = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_copy(heap, &out, val);

    // track reference
    if (val.binID == MATTE_VALUE_TYPE_OBJECT || val.binID == MATTE_VALUE_TYPE_FUNCTION) {
        object_link_parent_value(heap, m, &val);
    }
    if (newV->binID == MATTE_VALUE_TYPE_OBJECT || newV->binID == MATTE_VALUE_TYPE_FUNCTION) {
        object_unlink_parent_value(heap, m, newV);
    }

    matte_heap_recycle(heap, *newV);
    *newV = out;
}


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy_(matteHeap_t * heap, matteValue_t * to, matteValue_t from) {

        

    switch(from.binID) {
      case MATTE_VALUE_TYPE_EMPTY:
      case MATTE_VALUE_TYPE_NUMBER: 
      case MATTE_VALUE_TYPE_TYPE: 
      case MATTE_VALUE_TYPE_BOOLEAN: 
      case MATTE_VALUE_TYPE_STRING: 
        *to = from;
        return;

      case MATTE_VALUE_TYPE_FUNCTION:
      case MATTE_VALUE_TYPE_OBJECT: {
        if (to->binID    == from.binID &&
            to->value.id == from.value.id
            ) return;

        matte_heap_recycle(heap, *to);
        *to = from;
        return;
      }
    }
}

uint32_t matte_value_type_get_typecode(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_TYPE) return 0;
    return v.value.id;
}


int matte_value_isa(matteHeap_t * heap, matteValue_t v, matteValue_t typeobj) {
    if (typeobj.binID != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_cstring(heap->vm, "VM error: cannot query isa() with a non Type value.");
        return 0;
    }
    if (typeobj.value.id == heap->type_any.value.id) return 1;
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_value_get_type(heap, v).value.id == typeobj.value.id;
    } else {
        matteValue_t typep = matte_value_get_type(heap, v);
        if (typep.value.id == typeobj.value.id) return 1;

        MatteTypeData * d = &matte_array_at(heap->typecode2data, MatteTypeData, typep.value.id);
        if (!d->isa) return 0;

        uint32_t i;
        uint32_t count = matte_array_get_size(d->isa);
        for(i = 0; i < count; ++i) {
            if (matte_array_at(d->isa, uint32_t, i) == typeobj.value.id) return 1;
        }
        return 0;
    }
}

matteValue_t matte_value_type_name(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_cstring(heap->vm, "VM error: cannot get type name of non Type value.");
        return heap->specialString_empty;
    }
    switch(v.value.id) {
      case 1:     return heap->specialString_type_empty;
      case 2:     return heap->specialString_type_boolean;
      case 3:     return heap->specialString_type_number;
      case 4:     return heap->specialString_type_string;
      case 5:     return heap->specialString_type_object;
      case 6:     return heap->specialString_type_function;
      case 7:     return heap->specialString_type_type;
      default: { 
        if (v.value.id >= matte_array_get_size(heap->typecode2data)) {
            matte_vm_raise_error_cstring(heap->vm, "VM error: no such type exists...");                    
        } else
            return matte_array_at(heap->typecode2data, MatteTypeData, v.value.id).name;
      }
    }
    return heap->specialString_nothing;
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
const matteValue_t * matte_heap_get_function_type(matteHeap_t * h) {
    return &h->type_function;
}
const matteValue_t * matte_heap_get_type_type(matteHeap_t * h) {
    return &h->type_type;
}
const matteValue_t * matte_heap_get_any_type(matteHeap_t * h) {
    return &h->type_any;
}

matteValue_t matte_value_get_type(matteHeap_t * heap, matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY:   return heap->type_empty;
      case MATTE_VALUE_TYPE_BOOLEAN: return heap->type_boolean;
      case MATTE_VALUE_TYPE_NUMBER: return heap->type_number;
      case MATTE_VALUE_TYPE_STRING: return heap->type_string;
      case MATTE_VALUE_TYPE_TYPE: return heap->type_type;
      case MATTE_VALUE_TYPE_FUNCTION: return heap->type_function;
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        matteValue_t out;
        out.binID = MATTE_VALUE_TYPE_TYPE;
        out.value.id = m->table.typecode;
        return out;
      }

    }
    // error out?
    return heap->type_empty;
}




// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle_(
    matteHeap_t * heap,
    matteValue_t v
#ifdef MATTE_DEBUG__HEAP
    ,const char * file_,
    int line_
#endif    
) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT || v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->sortedHeap, v.value.id);
        if (!m) return; // normal for cycles
        // already handled as part of a rootless sweep.

        if (m->isRootless) return;
        if (m->toSweep) return;

        m->toSweep = 1;
        matte_array_push(heap->toSweep, m);
    } 
}





// checks to see if the object has a reference back to its parents
static int matte_object_check_ref_path_root(matteHeap_t * h, matteObject_t * m) {

    if (m->isRootRef) return 1;
    if (m->verifiedRoot == h->verifiedCycle) return 1;
    
    matte_array_push(h->routePather, m);
    uint8_t CHECKED = 21+h->pathCheckedPool++;
    m->routeChecked = CHECKED;
    uint32_t i;
    uint32_t len;
    uint32_t size = 1;
    MatteHeapParentChildLink * iter;
    while(size) {
        matteObject_t * current = matte_array_at(h->routePather, matteObject_t *, --size);
        
        // NOTE: preserver objects should ONLY accept true roots 
        // since a preserver's child will have its verifiedRoot by checked.
        // 
        if (current->isRootRef || (current->verifiedRoot == h->verifiedCycle)) {
            m->verifiedRoot = h->verifiedCycle;
            matte_array_set_size(h->routePather, 0);
            return 1;
        }
        
        
        // if referenced by preserver, keep it but you also need to check again later.
        // Also, only do this check if this object is not itself a preserver, else 
        // circular dependency chain for preservers is possible.
        if (current != m && current->hasPreserver && !m->hasPreserver) {
            matte_array_set_size(h->routePather, 0);
            

            // mark for recycle check next cycle
            if (!m->toSweep) {
                m->toSweep = 1;
                matte_array_push(h->toSweep, m);
            }   
            
            if (!current->toSweep) {
                current->toSweep = 1;
                matte_array_push(h->toSweep, current);            
            }
            return 1;
        }


        len = matte_array_get_size(current->refParents);
        iter = matte_array_get_data(current->refParents);
        matte_array_set_size(h->routePather, size+len);
        for(i = 0; i < len; ++i , ++iter) {
            uint32_t nc = iter->id;
            current = matte_bin_fetch(h->sortedHeap, nc);
            if (current->routeChecked != CHECKED) {
                current->routeChecked = CHECKED;
                #ifdef MATTE_DEBUG__HEAP
                    assert(matte_bin_fetch(h->sortedHeap, nc));
                #endif

                matte_array_at(h->routePather, matteObject_t *, size++) = current;
            }                        
        }

    }

    matte_array_set_size(h->routePather, 0);
    matteObject_t * current = m;
    if (m->hasPreserver) {
        matteValue_t preserver = matte_value_object_access(h, current->table.attribSet, h->specialString_preserver, 0);        
        m->hasPreserver = 0;
        if (preserver.binID) {
            m->verifiedRoot = h->verifiedCycle;
            h->preserver = 1;

            // you'll want to mark here, as the preserver can add/remove 
            // children / mess with child refs when the children may have already been 
            // marked for deletion
            #ifdef MATTE_DEBUG__HEAP
                printf("----Preserver called for %d. Reachable objects have been temporarily halted\n", current->heapID);
            #endif
            matteValue_t v = matte_vm_call(h->vm, preserver, matte_array_empty(), matte_array_empty(), NULL);


            matte_heap_recycle(h, preserver);
            matte_heap_recycle(h, v);


            // mark for recycle check next cycle
            if (!current->toSweep) {
                current->toSweep = 1;
                matte_array_push(h->toSweep, current);
            }            
            return 1;
        } else {
            // mark for recycle check next cycle
            if (!current->toSweep) {
                current->toSweep = 1;
                matte_array_push(h->toSweep, current);
            }                    
        }
    }
    return 0;
}

// add all objects in the group / cycle to the special rootless bin.
// There they will be handled safely (and separately) from the 
// normal sweep.
static void matte_object_group_add_to_rootless_bin(matteHeap_t * h, matteObject_t * m) {
    if (!m->isRootless) {
        m->isRootless = 1;
        matte_array_push(h->toSweep_rootless, m);
    }
}


static void object_cleanup(matteHeap_t * h) {
    
    matteTableIter_t * iter = matte_table_iter_create();
    uint32_t o;
    uint32_t olen = matte_array_get_size(h->toSweep_rootless);
    // might have been saved by finalizer
    for(o = 0; o < olen; ++o) {
        matteObject_t * m = matte_array_at(h->toSweep_rootless, matteObject_t *, o);
        if (m->dormant) continue;

        if (m->toSweep) {
            // need to remove from sweep list :(
            uint32_t i;
            uint32_t len = matte_array_get_size(h->toSweep);
            for(i = 0; i < len; ++i) {
                if (matte_array_at(h->toSweep, matteObject_t *, i) == m) {
                    matte_array_remove(h->toSweep, i);
                    break;
                }
            }
            m->toSweep = 0;
        }
        #ifdef MATTE_DEBUG__HEAP

            assert(!m->isRootRef);
            printf("--RECYCLING OBJECT %d\n", m->heapID);

            {
                matteValue_t vl;
                vl.binID = m->isFunction ? MATTE_VALUE_TYPE_FUNCTION : MATTE_VALUE_TYPE_OBJECT;
                vl.value.id = m->heapID;
                matte_heap_track_out(h, vl, "<removal>", 0);
            }
        #endif



        m->isRootless = 0;
        m->dormant = 1;
        if (matte_array_get_size(m->refChildren)) {
            matteValue_t v;
            v.binID = MATTE_VALUE_TYPE_OBJECT;

            uint32_t i;
            uint32_t len = matte_array_get_size(m->refChildren);
            MatteHeapParentChildLink * iter = matte_array_get_data(m->refChildren);
            for(i = 0; i < len; ++i , ++iter) {
                v.value.id = iter->id;
                matteObject_t * ch = matte_bin_fetch(h->sortedHeap, v.value.id);
                if (ch) {
                    object_unlink_parent_child_only_all(m, ch);
                    matte_heap_recycle(h, v);
                }
            }
            matte_array_set_size(m->refChildren, 0);
        }

        if (matte_array_get_size(m->refParents)) {
            matteValue_t v;
            v.binID = MATTE_VALUE_TYPE_OBJECT;

            uint32_t i;
            uint32_t len = matte_array_get_size(m->refParents);
            MatteHeapParentChildLink * iter = matte_array_get_data(m->refParents);
            for(i = 0; i < len; ++i , ++iter) {
                v.value.id = iter->id;
                matteObject_t * ch = matte_bin_fetch(h->sortedHeap, v.value.id);
                if (ch) {
                    object_unlink_parent_parent_only_all(ch, m);
                    matte_heap_recycle(h, v);
                }
            }
            matte_array_set_size(m->refParents, 0);
        }
        m->isRootRef = 0;



        if (m->isFunction) {
            m->function.stub = NULL;
            uint32_t n;
            uint32_t subl = matte_array_get_size(m->function.captures);
            // should be unlinked already because of above
            for(n = 0; n < subl; ++n) {
                matte_heap_recycle(h, matte_array_at(m->function.captures, CapturedReferrable_t, n).referrableSrc);
            }
            matte_array_set_size(m->function.captures, 0);
            matte_heap_recycle(h, m->function.origin);
            m->function.origin.binID = 0;
            matte_heap_recycle(h, m->function.origin_referrable);
            m->function.origin_referrable.binID = 0;
            if (m->function.types) {
                matte_array_destroy(m->function.types);
            }
            m->function.types = NULL;
            matte_bin_recycle(h->sortedHeap, m->heapID);
        
        } else {

            if (m->table.attribSet.binID) {
                matte_heap_recycle(h, m->table.attribSet);
                m->table.attribSet.binID = 0;
            }


            if (m->table.keyvalue_true.binID) {
                matte_heap_recycle(h, m->table.keyvalue_true);
                m->table.keyvalue_true.binID = 0;
            }
            if (m->table.keyvalue_false.binID) {
                matte_heap_recycle(h, m->table.keyvalue_false);
                m->table.keyvalue_false.binID = 0;
            }
            if (matte_array_get_size(m->table.keyvalues_number)) {
                uint32_t n;
                uint32_t subl = matte_array_get_size(m->table.keyvalues_number);
                for(n = 0; n < subl; ++n) {
                    matte_heap_recycle(h, matte_array_at(m->table.keyvalues_number, matteValue_t, n));
                }
                matte_array_set_size(m->table.keyvalues_number, 0);
            }

            if (!matte_table_is_empty(m->table.keyvalues_string)) {
                for(matte_table_iter_start(iter, m->table.keyvalues_string);
                    matte_table_iter_is_end(iter) == 0;
                    matte_table_iter_proceed(iter)
                ) {
                    matteValue_t * v = matte_table_iter_get_value(iter);
                    matte_heap_recycle(h, *v);
                    free(v);
                }
                matte_table_clear(m->table.keyvalues_string);
            }
            
            if (!matte_table_is_empty(m->table.keyvalues_types)) {

                for(matte_table_iter_start(iter, m->table.keyvalues_types);
                    matte_table_iter_is_end(iter) == 0;
                    matte_table_iter_proceed(iter)
                ) {
                    matteValue_t * v = matte_table_iter_get_value(iter);
                    matte_heap_recycle(h, *v);
                    free(v);
                }
                matte_table_clear(m->table.keyvalues_types);
            }
            if (!matte_table_is_empty(m->table.keyvalues_object)) {
                for(matte_table_iter_start(iter, m->table.keyvalues_object);
                    matte_table_iter_is_end(iter) == 0;
                    matte_table_iter_proceed(iter)
                ) {
                    matteValue_t * v = matte_table_iter_get_value(iter);
                    matte_heap_recycle(h, *v);
                    free(v);
                }
                matte_table_clear(m->table.keyvalues_object);
            }
            matte_bin_recycle(h->sortedHeap, m->heapID);
            if (m->table.nativeFinalizer) {
                m->table.nativeFinalizer(m->userdata, m->table.nativeFinalizerData);
                m->table.nativeFinalizer = NULL;
                m->table.nativeFinalizerData = NULL;
            }
        }
        // clean up object;
        m->userdata = NULL;


    }
    matte_table_iter_destroy(iter);
}

void matte_heap_push_lock_gc(matteHeap_t * h) {
    h->gcLocked++;
}
void matte_heap_pop_lock_gc(matteHeap_t * h) {
    h->gcLocked--;
}

void matte_heap_garbage_collect(matteHeap_t * h) {
    if (h->gcLocked) return;
    /*if (h->cooldown) {
        h->cooldown--;
        h->gcLocked = 0;
        return;
    }*/


    h->gcLocked = 1;
    ++(h->verifiedCycle);
    if (h->verifiedCycle == 0) h->verifiedCycle = 1;

    //matte_table_clear(h->verifiedRoot);
    // transfer to an array juuuust in case the 
    // finalizers mark additional objects for GCing
    matteArray_t * sweep = matte_array_clone(h->toSweep);
    matte_array_set_size(h->toSweep, 0);

    
    uint32_t i;
    uint32_t len = matte_array_get_size(sweep);
    #ifdef MATTE_DEBUG__HEAP
        if (len) {
            printf("BEGINNING SWEEP\n");
        }
    #endif

    for(i = 0; i < len; ++i) {
        matteObject_t * m = matte_array_at(sweep, matteObject_t *, i);
        m->toSweep = 0;
        m->verifiedRoot = 0;
    }

    for(i = 0; i < len; ++i) {
        matteObject_t * m = matte_array_at(sweep, matteObject_t *, i);
        if (m->isRootless) continue;


        if (!matte_object_check_ref_path_root(h, m)) {
            matte_object_group_add_to_rootless_bin(h, m);
        } 
        #ifdef MATTE_DEBUG__HEAP
            else {
                if (m->isRootRef)
                    printf("--IGNORING OBJECT %d (root object)\n", m->heapID);
                else
                    printf("--IGNORING OBJECT %d (still reachable)\n", m->heapID);
            }
        #endif
    }
    matte_array_destroy(sweep);

    // then we also check the rootless sweeping bin.
    // we limit the size to 100 objects per iteration.
    //#define ROOTLESS_SWEEP_LIMIT_COUNT 100

    len = matte_array_get_size(h->toSweep_rootless);
    if (len) {
        //if (len > ROOTLESS_SWEEP_LIMIT_COUNT) len = ROOTLESS_SWEEP_LIMIT_COUNT;
        do {
            h->preserver = 0;
            for(i = 0; i < len; ++i) {
                matteObject_t * m = matte_array_at(h->toSweep_rootless, matteObject_t *, i);
                if (matte_object_check_ref_path_root(h, m)) {
                    matte_array_remove(h->toSweep_rootless, i);
                    len--;
                    m->isRootless = 0;                

                }
            }
        } while (h->preserver);

        object_cleanup(h);
        matte_array_set_size(
            h->toSweep_rootless,
            0
        );
    }
    
    h->gcLocked = 0;
    //h->cooldown++;
}





#ifdef MATTE_DEBUG__HEAP 


typedef struct {
    matteValue_t refid;
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
    heap_track_info->values[MATTE_VALUE_TYPE_OBJECT] = matte_table_create_hash_pointer();
    heap_track_info->values[MATTE_VALUE_TYPE_FUNCTION] = matte_table_create_hash_pointer();

}

static const char * get_track_heap_symbol(int i) {
    switch(i) {
      case 0: return "--";
      case 1: return "++";
      case 2: return "->";
    }
    return "";
}


static void matte_heap_track_print(matteHeap_t * heap, uint32_t id) {
    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[MATTE_VALUE_TYPE_OBJECT], id);
    if (!inst) {
        inst = matte_table_find_by_uint(heap_track_info->values[MATTE_VALUE_TYPE_FUNCTION], id);
        if (!inst) {
            printf("No such object / value.\n");
            return;
        }
    }

    matte_value_print(heap, inst->refid);
    uint32_t i;
    uint32_t len = matte_array_get_size(inst->historyFiles);
    for(i = 0; i < len; ++i) {
        printf(
            "%s %s:%d\n", 
            get_track_heap_symbol(matte_array_at(inst->historyIncr, int, i)),
            matte_array_at(inst->historyFiles, char *, i),
            matte_array_at(inst->historyLines, int, i)
        );
    }
}


static void matte_heap_report_val(matteHeap_t * heap, matteTable_t * t) {
    matteTableIter_t * iter = matte_table_iter_create();
    for(matte_table_iter_start(iter, t);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        MatteHeapTrackInfo_Instance * inst = matte_table_iter_get_value(iter);
        if (!inst->refCount) continue;
        matte_value_print(heap, inst->refid);
        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            printf(
                "%s %s:%d\n", 
                get_track_heap_symbol(matte_array_at(inst->historyIncr, int, i)),
                matte_array_at(inst->historyFiles, char *, i),
                matte_array_at(inst->historyLines, int, i)
            );
        }
    }
}



matteValue_t matte_heap_track_in(matteHeap_t * heap, matteValue_t val, const char * file, int line) {
    if (!heap_track_info) {
        matte_heap_track_initialize();
    }
    if (!val.binID ||!(val.binID == MATTE_VALUE_TYPE_FUNCTION ||
                       val.binID == MATTE_VALUE_TYPE_OBJECT)) return val;
    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[val.binID], val.value.id);

    // reference is complete. REMOVE!!!
    if (inst && !inst->refCount) {
        matte_array_destroy(inst->historyLines);
        matte_array_destroy(inst->historyIncr);
        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            free(matte_array_at(inst->historyFiles, char *, i));
        }
        matte_array_destroy(inst->historyFiles);
        matte_table_remove_by_uint(heap_track_info->values[val.binID], val.value.id);
        free(inst);
        inst = NULL;
    } 

    if (!inst) {
        inst = calloc(sizeof(MatteHeapTrackInfo_Instance), 1);
        inst->historyFiles = matte_array_create(sizeof(char *));
        inst->historyLines = matte_array_create(sizeof(int));
        inst->historyIncr  = matte_array_create(sizeof(int));
        matte_table_insert_by_uint(heap_track_info->values[val.binID], val.value.id, inst);
        heap_track_info->unresolved++;
        inst->refid = val;
    } 

    char * fdup = strdup(file);
    int t = 1;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);
    inst->refCount++;
    return val;
}

void matte_heap_track_neutral(matteHeap_t * heap, matteValue_t val, const char * file, int line) {
    if (!heap_track_info) {
        matte_heap_track_initialize();
    }
    if (!val.binID ||!(val.binID == MATTE_VALUE_TYPE_FUNCTION ||
                       val.binID == MATTE_VALUE_TYPE_OBJECT)) return;
    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[val.binID], val.value.id);

    char * fdup = strdup(file);
    int t = 2;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);


}


void matte_heap_track_out(matteHeap_t * heap, matteValue_t val, const char * file, int line) {
    if (!heap_track_info) {
        matte_heap_track_initialize();
    }
    if (!val.binID ||!(val.binID == MATTE_VALUE_TYPE_FUNCTION ||
                       val.binID == MATTE_VALUE_TYPE_OBJECT)) return;
    MatteHeapTrackInfo_Instance * inst = matte_table_find_by_uint(heap_track_info->values[val.binID], val.value.id);
    if (!inst) {
        assert(!"If this assertion fails, something has used a stray / uninitialized value and tried to distroy it.");
    }

    char * fdup = strdup(file);
    int t = 0;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);


    if (inst->refCount == 0) {
        printf("---REFCOUNT NEGATIVE DETECTED---\n");
        if (inst->refid.binID == MATTE_VALUE_TYPE_OBJECT || inst->refid.binID == MATTE_VALUE_TYPE_FUNCTION)
            matte_value_print(heap, inst->refid);
        else 
            printf("Value: {%d, %d}\n", inst->refid.binID, inst->refid.value.id);


        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            printf(
                "%s %s:%d\n", 
                get_track_heap_symbol(matte_array_at(inst->historyIncr, int, i)),
                matte_array_at(inst->historyFiles, char *, i),
                matte_array_at(inst->historyLines, int, i)
            );
        }
        printf("---REFCOUNT NEGATIVE DETECTED---\n");
        assert(!"Negative refcount means something is getting rid of a value after it has been disposed.");
    }

    inst->refCount--;
    
        
    if (inst->refCount == 0) {
        heap_track_info->resolved++;
        heap_track_info->unresolved--;
    }

}

// prints a report of the heap if any references have not 
// 
int matte_heap_report(matteHeap_t * heap) {
    if (!heap_track_info) return 0;
    if (!heap_track_info->unresolved) return 0;

    printf("+---- Matte Heap Value Tracker ----+\n");
    printf("|     UNFREED VALUES DETECTED      |\n");
    printf("Object values:\n"); matte_heap_report_val(heap, heap_track_info->values[MATTE_VALUE_TYPE_OBJECT]);
    printf("Function values:\n"); matte_heap_report_val(heap, heap_track_info->values[MATTE_VALUE_TYPE_FUNCTION]);
    printf("|                                  |\n");
    printf("+----------------------------------+\n\n");



}



#endif
