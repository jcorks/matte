/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
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
#include "matte_store.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_pool.h"
#include "matte_au32.h"
#include "matte_string.h"
#include "matte_bytecode_stub.h"
#include "matte_vm.h"
#include "matte_store_string.h"
#include "matte.h"
#include "matte_mvt2.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
#ifdef MATTE_DEBUG 
    #include <assert.h>
#endif

#define ROOT_AGE_LIMIT 2

#define MATTE_PI 3.14159265358979323846

// for the private binding calls.
matteValue_t matte_vm_call_full(
    matteVM_t * vm, 
    matteValue_t func, 
    matteValue_t privateBinding,
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
);



// from OS features
double matte_os_get_ticks();



typedef struct {
    // name of the type
    matteValue_t name;
    
    // All the typecodes that this type can validly be used as in an 
    // isa comparison
    matteArray_t * isa;    
    
    // The static members for a custom type.
    matteTable_t * layout;
} MatteTypeData;

typedef struct matteObject_t matteObject_t;





typedef struct matteStoreBin_t matteStoreBin_t;

// creates a new store bin, which holds and generates in-memory 
// object references. Instead of being freed, the bin acts as 
// an object pool, holding references to functions and tables.
// functions always have even storeIDs, and tables always odd
static matteStoreBin_t * matte_store_bin_create();

// Fetches a free table object.
static matteObject_t * matte_store_bin_add_table(matteStoreBin_t *);

// Fetches a free function object.
static matteObject_t * matte_store_bin_add_function(matteStoreBin_t *);

// fetches the object from the given ID, returning NULL if there is no such object.
static matteObject_t * matte_store_bin_fetch(matteStoreBin_t *, uint32_t id);

static matteObject_t * matte_store_bin_fetch_function(matteStoreBin_t * store, uint32_t id);
static matteObject_t * matte_store_bin_fetch_table(matteStoreBin_t * store, uint32_t id);


// recycles an object.
static void matte_store_bin_recycle(matteStoreBin_t *, uint32_t id);

static void matte_store_bin_destroy(matteStoreBin_t *);






#define IS_FUNCTION_ID(__ID__) (((__ID__)/2)*2 == (__ID__))
#define IS_FUNCTION_OBJECT(__OBJ__) (((__OBJ__)->storeID/2)*2 == (__OBJ__)->storeID)

			
typedef struct {
    uint32_t next;
    uint32_t data;
} matteObjectNode_t;


enum {
    ROOT_AGE__YOUNG,
    ROOT_AGE__OLDEST
};




/// Gets a pointer to the special referrable value in question.
/// If none, NULL is returned.
/// Can raise error.
/*
    It can refer to an argument, local value, or captured variable.
    0 -> context object
    1, num args -1 -> arguments (this is the number of arguments that the function expected.)
    num args, num args + num locals - 1 -> locals 
    num args + num locals, num args + num locals + num captured -1 -> captured
*/
// Caller is responsible for recycling value.
matteValue_t * matte_vm_stackframe_get_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID);

void matte_vm_stackframe_set_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID, matteValue_t v);

/// Functions for a built-in references. These are locked into the store 
matteValue_t * matte_vm_get_external_builtin_function_as_value(
    matteVM_t * vm,
    matteExtCall_t ext
);





struct matteStore_t {    
    matteStoreBin_t * bin;
    matteArray_t * valueStore;
    matteArray_t * valueStore_dead;


    // 0 -> white 
    // 1 -> grey 
    // 2 -> black
    uint32_t tricolor[3];
    
    uint16_t gcLocked;
    int pathCheckedPool;

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
    matteValue_t type_nullable;
    
    
    matteTable_t * type_number_methods;
    matteTable_t * type_string_methods;
    matteTable_t * type_object_methods;
    
    // names for all types, on-demand
    // MatteTypeData
    matteArray_t * typecode2data;
    matteArray_t * external;
    matteArray_t * kvIter_v;
    matteArray_t * kvIter_k;
    matteArray_t * mgIter;
    matteArray_t * routePather;
    matteTableIter_t * routeIter;
    // all objects contained in here are "verified as root-reachable"
    // for the reamainder of the root sweep.
    //matteTable_t * verifiedRoot;
    uint16_t verifiedCycle;
    uint16_t cooldown;
    int shutdown;
    uint32_t gcRequestStrength;
    uint32_t gcOldCycle;
    uint32_t pendingRoots;
    matteVM_t * vm;

    matteStringStore_t * stringStore;
    
    
    // string value for "empty"
    matteValue_t specialString_empty;
    matteValue_t specialString_nothing;
    matteValue_t specialString_true;
    matteValue_t specialString_false;
    matteValue_t specialString_key;
    matteValue_t specialString_value;
    matteValue_t specialString_dynamicBindToken;
    
    
    matteValue_t specialString_type_empty;
    matteValue_t specialString_type_boolean;
    matteValue_t specialString_type_number;
    matteValue_t specialString_type_string;
    matteValue_t specialString_type_object;
    matteValue_t specialString_type_function;
    matteValue_t specialString_type_type;
    matteValue_t specialString_type_any;
    matteValue_t specialString_type_nullable;
    
    matteValue_t specialString_get;
    matteValue_t specialString_set;
    matteValue_t specialString_bracketAccess;
    matteValue_t specialString_dotAccess;
    matteValue_t specialString_foreach;
    matteValue_t specialString_keys;
    matteValue_t specialString_values;
    matteValue_t specialString_name;
    matteValue_t specialString_inherits;
    
    
    
    mattePool_t * nodes;

    // multipurpose iterator so no need to 
    // create an ephemeral one
    matteTableIter_t * freeIter;
    
    matteArray_t * toRemove;
    uint32_t roots;
    double ticksGC;

    
    #ifdef MATTE_DEBUG__STORE 
        uint64_t gcCycles;
    #endif
};


// When capturing values, the base 
// referrable of the stackframe is saved along 
// with an index into it pointing to which 
// value it is. This allows upding the real, singular 
// location of it when updating it but also allows 
// fetching copies as needed.
typedef struct {
    uint32_t functionID;
    uint32_t index;
} CapturedReferrable_t;

enum {
    // whether the object is active. When inactive, the 
    // object is waiting in a bin for reuse.
    OBJECT_STATE__RECYCLED = 1,
    
    // whether the object has an interface.
    OBJECT_STATE__HAS_INTERFACE = 2,
    
    // Whether the object is a Record.
    // Records.... 
    // - have a static set of string-only keys that are set before setting as a record.
    // - Can only have string key read / writes
    OBJECT_STATE__HAS_LAYOUT = 4
};

#define ENABLE_STATE(__o__, __s__) ((__o__)->state |= (__s__))
#define DISABLE_STATE(__o__, __s__) ((__o__)->state &= ~(__s__))
#define QUERY_STATE(__o__, __s__) (((__o__)->state & (__s__)) != 0)


typedef struct {
    // any user data. Defaults to null
    void * userdata;

    void (*nativeFinalizer)(void * objectUserdata, void * functionUserdata);
    void * nativeFinalizerData;


} matteObjectExternalData_t;

typedef struct matteObjectChildNode_t matteObjectChildNode_t;
struct matteObjectChildNode_t {
    matteObjectChildNode_t * next;
    uint32_t dataID;
    uint32_t count;
};


typedef struct {
    // call captured variables. Defined at function creation.
    matteValue_t ** captures;

    // in order, the origin functions for each capture
    uint32_t * captureOrigins;

    // list of THIS functions referrables. This is 
    // owned and freed by the function
    matteValue_t * referrables;    
} matteVariableData_t;


struct matteObject_t {
    

    //matteTable_t * refChildren;
    //matteTable_t * refParents;
    uint32_t children;
    uint32_t storeID;

    uint32_t typecode;
    // id within sorted store.
    uint16_t rootState;
    uint8_t  state;
    uint8_t  color;

    uint32_t prevRoot;
    uint32_t nextRoot;
    
    uint32_t prevColor;
    uint32_t nextColor;

    
    #ifdef MATTE_DEBUG__STORE
        matteArray_t * parents;
        uint32_t fileIDsrc;
        uint32_t fileLine;
        uint64_t gcCycles;
    #endif
    matteObjectExternalData_t * ext; 
    
    union {
        struct {
            matteArray_t  * keyvalues_number;


            // matteValue_t -> matteValue_t
            matteMVT2_t * keyvalues_id;

            matteValue_t * attribSet;
            matteValue_t * privateBinding;

            
        } table;
        
        struct {
            // stub for functions. If null, is a normal object.
            matteBytecodeStub_t * stub;

            // for type-strict functions, these are the function types 
            // asserted for args and return values.
            matteArray_t * types;

            // for function objects, a reference to the original 
            // creation context is kept. This is because referrables 
            // can track back to original creation contexts.
            uint32_t origin;

            uint16_t isCloned;
            uint16_t referrablesCount;


            matteVariableData_t * vars;
        } function;
    };

};




//////////////////////////////////////
//// See matte_store__gc

    enum {
        OBJECT_TRICOLOR__WHITE,
        OBJECT_TRICOLOR__GREY,
        OBJECT_TRICOLOR__BLACK
    };


    // Performs garbage collection.
    // Should be called frequently to regularly
    // cleanup unused objects and data.
    void matte_store_garbage_collect(matteStore_t * h);

    // Adds an object to a tricolor group
    static void matte_store_garbage_collect__add_to_color(matteStore_t * h, matteObject_t * m);

    // Removes an object from a tricolor group
    static void matte_store_garbage_collect__rem_from_color(matteStore_t * h, matteObject_t * m);


///////////////////////////////////////

/*
static void print_list(matteStore_t * h) {
    matteObject_t * r = matte_store_bin_fetch(h->bin, h->roots);
    uint32_t count = 0;
    while(r) {

        //printf("{%d}-", r->storeID);
        count += 1;
        r = r->nextRoot;
        if (count > 10000)
            printf("whats up\n");

    }
    //printf("|\n");
    printf("!!!!!!!!!!%d\n", count);
}
*/


static void remove_root_node(matteStore_t * h, uint32_t * root, matteObject_t * m) {
    //print_list(h);
    if (*root == m->storeID) { // no prev
        *root = m->nextRoot;
        if (*root)
            matte_store_bin_fetch(h->bin, (*root))->prevRoot = 0;
    } else {
        if (m->prevRoot) {
            matte_store_bin_fetch(h->bin, m->prevRoot)->nextRoot = m->nextRoot;
        }
        
        if (m->nextRoot) {
            matte_store_bin_fetch(h->bin, m->nextRoot)->prevRoot = m->prevRoot;
        }
    }
    //print_list(h);
    m->nextRoot = 0;
    m->prevRoot = 0;
}






static void store_recycle_value_pointer(matteStore_t * h, uint32_t id) {
    matte_array_push(h->valueStore_dead, id);
}

static void store_free_value_pointers(matteStore_t * h) {
    matte_array_destroy(h->valueStore);
    matte_array_destroy(h->valueStore_dead);
}

static matteValue_t * object_lookup(matteStore_t * store, matteObject_t * m, matteValue_t key) {
    if (QUERY_STATE(m, OBJECT_STATE__HAS_LAYOUT)) {
        MatteTypeData * data = &matte_array_at(store->typecode2data, MatteTypeData, m->typecode);

        if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * err = matte_string_create_from_c_str(
                "%s",
                "Objects with layouts can only have string keys."
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL;                
        } else if (!matte_table_find_by_uint(data->layout, (key.value.id))) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object has no such key '%s' in its layout.",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL;                
        }
    }



    switch(matte_value_type(key)) {
      case MATTE_VALUE_TYPE_EMPTY: return NULL;

      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        double valNum = matte_value_get_number(key);
        if (valNum < 0) return NULL;
        if (!m->table.keyvalues_number) return NULL;
        uint32_t index = (uint32_t)valNum;
        if (index < matte_array_get_size(m->table.keyvalues_number)) {
            return &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        } else {
            return NULL;
        }   
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN:
      case MATTE_VALUE_TYPE_STRING:
      case MATTE_VALUE_TYPE_TYPE:
      case MATTE_VALUE_TYPE_OBJECT:
        if (!m->table.keyvalues_id) return NULL;
        return matte_mvt2_find(m->table.keyvalues_id, key);
    }
    return NULL;
}







static void object_link_parent(matteStore_t * h, matteObject_t * parent, matteObject_t * child) {
    #ifdef MATTE_DEBUG__STORE
        matteValue_t v;
        v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
        v.value.id = parent->storeID;
        matte_array_push(child->parents, v);
    #endif
    
    
    if (parent->color == OBJECT_TRICOLOR__BLACK || parent->rootState) {
        if (child->rootState == 0) {
            matte_store_garbage_collect__rem_from_color(h, child);
            child->color = OBJECT_TRICOLOR__GREY;
            matte_store_garbage_collect__add_to_color(h, child);
        }
    }
    if (!parent->children) {
        parent->children = matte_pool_add(h->nodes);
        matteObjectNode_t * n = matte_pool_fetch(h->nodes, matteObjectNode_t, parent->children);
        n->next = 0;
        n->data = child->storeID;
    } else {
        uint32_t next = matte_pool_add(h->nodes);
        matteObjectNode_t * n = matte_pool_fetch(h->nodes, matteObjectNode_t, next);
        matteObjectNode_t * old = matte_pool_fetch(h->nodes, matteObjectNode_t, parent->children);
        
        n->next = parent->children;
        n->data = child->storeID;
        
        parent->children = next;
    }
}

static void object_unlink_parent(matteStore_t * h, matteObject_t * parent, matteObject_t * child) {
    if (child == NULL) return; // when? only when root check early refcount = 0 forced cleanup?


    #ifdef MATTE_DEBUG__STORE
    {
        uint32_t i;
        int found = 0;
        for(i = 0; i < matte_array_get_size(child->parents); ++i) {
            if (matte_array_at(child->parents, matteValue_t, i).value.id == parent->storeID) {
                matte_array_remove(child->parents, i);
                found = 1;
                break;            
            }
        }
        
        assert(found);
    }
    #endif


    uint32_t next = parent->children;
    matteObjectNode_t * prev = NULL;
    while(next) {
        matteObjectNode_t * node = matte_pool_fetch(h->nodes, matteObjectNode_t, next);
        if (node->data == child->storeID) {            
            if (prev) {
                prev->next = node->next;
            }
            
            if (parent->children == next)
                parent->children = node->next;
            
            node->next = 0;
            matte_pool_recycle(h->nodes, next);
            break;
        }
        next = node->next;
        prev = node;
    }

    
    /*
    if (child->refcount) {
        child->refcount--; 
        if (child->refcount == 0) {
            if (child->color != OBJECT_TRICOLOR__WHITE) {
                if (child->rootState == 0) {
                    matte_store_garbage_collect__rem_from_color(h, child);
                    child->color = OBJECT_TRICOLOR__WHITE;
                    matte_store_garbage_collect__add_to_color(h, child);
                }
            }        
        }
        // TODO: change color of child? next cycle ok? 
    }
    */
    /*
        if (child->refcount)
            child->refcount--; 
        
    // absolutely ready for cleanup
        if (!child->refcount && !child->rootState) {
            if (!QUERY_STATE(child, OBJECT_STATE__IS_ETERNAL)) {
                if (!QUERY_STATE(child, OBJECT_STATE__TO_REMOVE)) {
                    matte_array_push(h->toRemove, child);
                    ENABLE_STATE(child, OBJECT_STATE__TO_REMOVE);
                }            
            } 
        }
    */
}




static void object_link_parent_value_(matteStore_t * store, matteObject_t * parent, const matteValue_t * child) {
    object_link_parent(store, parent, matte_store_bin_fetch(store->bin, child->value.id));
}

static void object_unlink_parent_value_(matteStore_t * store, matteObject_t * parent, const matteValue_t * child) {
    if (matte_value_type(*child) != MATTE_VALUE_TYPE_OBJECT) return;
    object_unlink_parent(store, parent, matte_store_bin_fetch(store->bin, child->value.id));
}

#ifdef MATTE_DEBUG__STORE
#define object_link_parent_value(__STORE__, __PARENT__, __CHILD__) (matte_store_track_in_lock(__STORE__, *(__CHILD__), __FILE__, __LINE__)); object_link_parent_value_(__STORE__, __PARENT__, __CHILD__);
#define object_unlink_parent_value(__STORE__, __PARENT__, __CHILD__) (matte_store_track_out_lock(__STORE__, *(__CHILD__), __FILE__, __LINE__)); object_unlink_parent_value_(__STORE__, __PARENT__, __CHILD__);
#else
#define object_link_parent_value(__STORE__, __PARENT__, __CHILD__) object_link_parent_value_(__STORE__, __PARENT__, __CHILD__);
#define object_unlink_parent_value(__STORE__, __PARENT__, __CHILD__)  object_unlink_parent_value_(__STORE__, __PARENT__, __CHILD__);

#endif


static matteValue_t * object_put_prop(matteStore_t * store, matteObject_t * m, matteValue_t key, matteValue_t val) {
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_copy(store, &out, val);


    if (QUERY_STATE(m, OBJECT_STATE__HAS_LAYOUT)) {
        if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * err = matte_string_create_from_c_str(
                "%s",
                "Objects with layouts can only have string keys."
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return NULL;                
        }
        
        uint32_t typecode = (uint32_t)(uintptr_t)matte_table_find_by_uint(
            matte_array_at(store->typecode2data, MatteTypeData, m->typecode).layout,
            key.value.id 
        );
        
        if (typecode == 0) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object has no such key '%s' in its layout.",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return NULL;    
        }
        
        matteValue_t typeVal = {};
        typeVal.binIDreserved = MATTE_VALUE_TYPE_TYPE;
        typeVal.value.id = typecode;
        if (!matte_value_isa(store, val, typeVal)) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object member '%s' cannot be set from a %s. Any setting must be of type %s.",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key)),
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, val)))),
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, typeVal))) 
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return NULL;            
        }
    }


    if (matte_value_type(key) == MATTE_VALUE_TYPE_EMPTY) return NULL;


    // track reference
    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {    
        object_link_parent_value(store, m, &val);
    }

    switch(matte_value_type(key)) {
      case MATTE_VALUE_TYPE_STRING: {
        if (!m->table.keyvalues_id) m->table.keyvalues_id = matte_mvt2_create();
        matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);
        if (value) {
            if (matte_value_type(*value) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, m, value);
            }
            matte_store_recycle(store, *value);
            matte_mvt2_insert(m->table.keyvalues_id, key, out);
            return matte_mvt2_find(m->table.keyvalues_id, key);
        } else {
            
            matte_string_store_ref_id(store->stringStore, key.value.id);
            return matte_mvt2_insert(m->table.keyvalues_id, key, out);
        }
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        double valNum = matte_value_get_number(key);
        if (valNum < 0) {
            matte_vm_raise_error_cstring(store->vm, "Objects cannot have Number keys that are negative. Try using a string instead.");
            matte_store_recycle(store, out);
            return NULL;
        }
        if (!m->table.keyvalues_number) m->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));
        uint32_t index = (uint32_t)valNum;
        if (index >= matte_array_get_size(m->table.keyvalues_number)) {
            uint32_t currentLen = matte_array_get_size(m->table.keyvalues_number);
            matte_array_set_size(m->table.keyvalues_number, index+1);
            uint32_t i;
            // new entries as empty
            for(i = currentLen; i < index+1; ++i) {
                matte_array_at(m->table.keyvalues_number, matteValue_t, i) = matte_store_new_value(store);
            }
        }

        matteValue_t * newV = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        if (matte_value_type(*newV) == MATTE_VALUE_TYPE_OBJECT) {
            object_unlink_parent_value(store, m, newV);
        }

        matte_store_recycle(store, *newV);
        *newV = out;
        return newV;
      }
      
      case MATTE_VALUE_TYPE_TYPE:
      case MATTE_VALUE_TYPE_OBJECT:
      case MATTE_VALUE_TYPE_BOOLEAN: {
        if (!m->table.keyvalues_id) m->table.keyvalues_id = matte_mvt2_create();        
        matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);        
        if (value) {
            if (matte_value_type(*value) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, m, value);
            }
            matte_store_recycle(store, *value);
            matte_mvt2_insert(m->table.keyvalues_id, key, out);
            return matte_mvt2_find(m->table.keyvalues_id, key);
        } else {
            object_link_parent_value(store, m, &key);
            return matte_mvt2_insert(m->table.keyvalues_id, key, out);
        }
      }
    }
    return NULL;
}

// returns a type conversion operator if it exists
static matteValue_t object_get_conv_operator(matteStore_t * store, matteObject_t * m, uint32_t type) {
    matteValue_t out = matte_store_new_value(store);
    if (m->table.attribSet == NULL || matte_value_type(*m->table.attribSet) == 0) return out;
    matteValue_t * operator_ = m->table.attribSet;

    matteValue_t key;
    store = store;
    key.binIDreserved = MATTE_VALUE_TYPE_TYPE;
    key.value.id = type;
    // OK: empty doesnt allocate anything
    out = matte_value_object_access(store, *operator_, key, 0);
    return out;
}

// isDot is either 0 ([]) or 1 (.) 
// read if either 0 (write) or 1(read)
static matteValue_t object_get_access_operator(matteStore_t * store, matteObject_t * m, int isBracket, int read) {
    matteValue_t out = matte_store_new_value(store);
    if (m->table.attribSet == NULL || matte_value_type(*m->table.attribSet) == 0) return out;


    
    matteValue_t set = matte_value_object_access(store, *m->table.attribSet, isBracket ? store->specialString_bracketAccess : store->specialString_dotAccess, 0);
    if (matte_value_type(set)) {
        if (matte_value_type(set) != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(store->vm, "operator['[]'] and operator['.'] property must be an Object if it is set.");
        } else {
            out = matte_value_object_access(store, set, read ? store->specialString_get : store->specialString_set, 0);
        }            
    }

    matte_store_recycle(store, set);            
    return out;
}

static uint32_t CREATED_COUNT = 1;



static const char * BUILTIN_NUMBER__NAMES[] = {
    "PI",
    "parse",
    "random",
    NULL
};

static int BUILTIN_NUMBER__IDS[] = {
    MATTE_EXT_CALL__NUMBER__PI,
    MATTE_EXT_CALL__NUMBER__PARSE,
    MATTE_EXT_CALL__NUMBER__RANDOM
};

static const char * BUILTIN_STRING__NAMES[] = {
    "combine",
    NULL
};

static int BUILTIN_STRING__IDS[] = {
    MATTE_EXT_CALL__STRING__COMBINE,
};


static const char * BUILTIN_OBJECT__NAMES[] = {
    "newType",
    "instantiate",
    "freezeGC",
    "thawGC",
    "garbageCollect",
    NULL
};


static int BUILTIN_OBJECT__IDS[] = {
    MATTE_EXT_CALL__OBJECT__NEWTYPE,
    MATTE_EXT_CALL__OBJECT__INSTANTIATE,
    MATTE_EXT_CALL__OBJECT__FREEZEGC,
    MATTE_EXT_CALL__OBJECT__THAWGC,
    MATTE_EXT_CALL__OBJECT__GARBAGECOLLECT
};


static void add_table_refs(
    matteTable_t * table,
    matteStringStore_t * stringStore,
    const char ** names,
    int * ids
) {
    while(*names) {


        
        matte_table_insert_by_uint(table, matte_string_store_ref_cstring(stringStore, *names), (void*)(uintptr_t)*ids);
    
        names++;
        ids++;
    }
}


matteStore_t * matte_store_create(matteVM_t * vm) {
    matteStore_t * out = (matteStore_t*)matte_allocate(sizeof(matteStore_t));
    out->vm = vm;
    out->bin = matte_store_bin_create();
    out->valueStore = matte_array_create(sizeof(matteValue_t));
    out->valueStore_dead = matte_array_create(sizeof(uint32_t));
    matteValue_t unused = {};
    matte_array_push(out->valueStore, unused);
    out->typecode2data = matte_array_create(sizeof(MatteTypeData));



    out->routePather = matte_array_create(sizeof(void*));
    out->routeIter = matte_table_iter_create();
    out->freeIter = matte_table_iter_create();
    out->nodes = matte_pool_create(sizeof(matteObjectNode_t), NULL);
    //out->verifiedRoot = matte_table_create_hash_pointer();
    out->stringStore = matte_string_store_create();
    out->toRemove = matte_array_create(sizeof(uint32_t));
    out->external = matte_array_create(sizeof(matteValue_t));
    out->kvIter_v = matte_array_create(sizeof(matteValue_t));
    out->kvIter_k = matte_array_create(sizeof(matteValue_t));
    out->pendingRoots = 0;

    MatteTypeData dummyD = {};
    matte_array_push(out->typecode2data, dummyD);

    srand(time(NULL));
    out->type_number_methods = matte_table_create_hash_pointer();
    out->type_object_methods = matte_table_create_hash_pointer();
    out->type_string_methods = matte_table_create_hash_pointer();

    matteValue_t dummy = {};
    out->type_empty = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_boolean = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_number = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_string = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_object = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_function = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_type = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_any = matte_value_create_type(out, dummy, dummy, dummy);
    out->type_nullable = matte_value_create_type(out, dummy, dummy, dummy);

    out->specialString_empty.value.id = matte_string_store_ref_cstring(out->stringStore, "empty");
    out->specialString_nothing.value.id = matte_string_store_ref_cstring(out->stringStore, "");
    out->specialString_true.value.id = matte_string_store_ref_cstring(out->stringStore, "true");
    out->specialString_false.value.id = matte_string_store_ref_cstring(out->stringStore, "false");
    out->specialString_dynamicBindToken.value.id = matte_string_store_ref_cstring(out->stringStore, "$");

    out->specialString_type_boolean.value.id = matte_string_store_ref_cstring(out->stringStore, "Boolean");
    out->specialString_type_empty.value.id = matte_string_store_ref_cstring(out->stringStore, "Empty");
    out->specialString_type_number.value.id = matte_string_store_ref_cstring(out->stringStore, "Number");
    out->specialString_type_string.value.id = matte_string_store_ref_cstring(out->stringStore, "String");
    out->specialString_type_object.value.id = matte_string_store_ref_cstring(out->stringStore, "Object");
    out->specialString_type_type.value.id = matte_string_store_ref_cstring(out->stringStore, "Type");
    out->specialString_type_function.value.id = matte_string_store_ref_cstring(out->stringStore, "Function");
    out->specialString_type_any.value.id = matte_string_store_ref_cstring(out->stringStore, "Any");
    out->specialString_type_nullable.value.id = matte_string_store_ref_cstring(out->stringStore, "Nullable");

    out->specialString_empty.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_nothing.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_true.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_false.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_dynamicBindToken.binIDreserved = MATTE_VALUE_TYPE_STRING;



    out->specialString_type_boolean.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_empty.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_number.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_string.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_object.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_function.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_type.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_any.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_type_nullable.binIDreserved = MATTE_VALUE_TYPE_STRING;


    out->specialString_set.value.id = matte_string_store_ref_cstring(out->stringStore, "set");
    out->specialString_set.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_get.value.id = matte_string_store_ref_cstring(out->stringStore, "get");
    out->specialString_get.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_bracketAccess.value.id = matte_string_store_ref_cstring(out->stringStore, "[]");
    out->specialString_bracketAccess.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_dotAccess.value.id = matte_string_store_ref_cstring(out->stringStore, ".");
    out->specialString_dotAccess.binIDreserved = MATTE_VALUE_TYPE_STRING;


    out->specialString_foreach.value.id = matte_string_store_ref_cstring(out->stringStore, "foreach");
    out->specialString_foreach.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_keys.value.id = matte_string_store_ref_cstring(out->stringStore, "keys");
    out->specialString_keys.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_values.value.id = matte_string_store_ref_cstring(out->stringStore, "values");
    out->specialString_values.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_name.value.id = matte_string_store_ref_cstring(out->stringStore, "name");
    out->specialString_name.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_inherits.value.id = matte_string_store_ref_cstring(out->stringStore, "inherits");
    out->specialString_inherits.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_key.value.id = matte_string_store_ref_cstring(out->stringStore, "key");
    out->specialString_key.binIDreserved = MATTE_VALUE_TYPE_STRING;
    out->specialString_value.value.id = matte_string_store_ref_cstring(out->stringStore, "value");
    out->specialString_value.binIDreserved = MATTE_VALUE_TYPE_STRING;
    
    add_table_refs(out->type_number_methods, out->stringStore, BUILTIN_NUMBER__NAMES, BUILTIN_NUMBER__IDS);
    add_table_refs(out->type_object_methods, out->stringStore, BUILTIN_OBJECT__NAMES, BUILTIN_OBJECT__IDS);
    add_table_refs(out->type_string_methods, out->stringStore, BUILTIN_STRING__NAMES, BUILTIN_STRING__IDS);

    out->ticksGC = matte_os_get_ticks();


    matte_value_object_push_lock(out, matte_store_empty_function(out));
    return out;
    
}

void matte_store_destroy(matteStore_t * h) {
    h->shutdown = 1;
    uint32_t i;
    uint32_t len = matte_array_get_size(h->external);
    for(i = 0; i < len; ++i) {
        matteValue_t val = matte_array_at(h->external, matteValue_t, i);
        matte_value_object_pop_lock(h, val);
    }

    matteObject_t * m = matte_store_bin_fetch_function(h->bin, 2);
    matte_deallocate(m->function.stub);

    matte_value_object_pop_lock(h, matte_store_empty_function(h));
    matte_array_destroy(h->external);
    
    while(
        h->tricolor[OBJECT_TRICOLOR__GREY] || 
        h->tricolor[OBJECT_TRICOLOR__WHITE]    
    )
        matte_store_garbage_collect(h);


    #ifdef MATTE_DEBUG__STORE
    assert(matte_store_report(h) == 0);
    #endif

    matte_store_bin_destroy(h->bin);
    len = matte_array_get_size(h->typecode2data);
    for(i = 0; i < len; ++i) {
        MatteTypeData d = matte_array_at(h->typecode2data, MatteTypeData, i);
        if (d.isa) {
            matte_array_destroy(d.isa);
        }
        
        if (d.layout) {
            matte_table_destroy(d.layout);
        }
    }
    matte_array_destroy(h->typecode2data);
    matte_array_destroy(h->routePather);
    matte_table_iter_destroy(h->routeIter);
    //matte_table_destroy(h->verifiedRoot);
    matte_string_store_destroy(h->stringStore);
    matte_table_destroy(h->type_number_methods);
    matte_table_destroy(h->type_object_methods);
    matte_table_destroy(h->type_string_methods);
    store_free_value_pointers(h);
    matte_array_destroy(h->toRemove);
    matte_table_iter_destroy(h->freeIter);
    matte_pool_destroy(h->nodes);

    matte_array_destroy(h->kvIter_v);
    matte_array_destroy(h->kvIter_k);

    matte_deallocate(h);
}


matteValue_t matte_store_new_value_(matteStore_t * h) {
    matteValue_t out;
    out.binIDreserved = 0;
    return out;
}

matteValue_t matte_store_empty_function(matteStore_t * h) {
    matteValue_t out;
    out.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    out.value.id = 2;
    return out;
}


void matte_value_into_empty_(matteStore_t * store, matteValue_t * v) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_EMPTY;
}

int matte_value_is_empty(matteStore_t * store, matteValue_t v) {
    return matte_value_type(v) == 0;
}


// Sets the type to a number with the given value



/*

// quick thing i wrote up to get the double layout and make sure 
// stuff was working. feel free to use.
void print_double(double d) {
    uint64_t * v = (uint64_t *)&d;
    
    #define SIGN 63
    #define EXPONENT_LENGTH 11
    #define FRACTION_LENGTH 52
    #define GET_NEXT_BIT() (iter--,((*v) & (one<<(iter+one))) != zero)
    
    uint64_t iter = SIGN;
    uint64_t one = 1;
    uint64_t zero = 0;
        
    printf("%d - ", GET_NEXT_BIT());
    
    // exponent
    int i;
    for(i = 0; i < EXPONENT_LENGTH; ++i) {
        printf("%d", GET_NEXT_BIT());
    }

    // fraction
    printf(" - ");
    for(i = 0; i < FRACTION_LENGTH; ++i) {
        printf("%d", GET_NEXT_BIT());
    }
    printf("\n");    
}
*/




/*
    Notes on the number implementation 
    
    To make values take up less space, the least significant 
    bit of the fraction of the double is reserved for whether or 
    not the value is a Number (as matteValue_t's are the size of a double)
    
    This is also the reason for the values of the enum MATTE_VALUE_TYPE_* 
    being non-linear while preserving that a "clear" value of matteValue_t 
    corresponds to the empty value.


*/

void matte_value_into_number_(matteStore_t * store, matteValue_t * v, double val) {
    matte_store_recycle(store, *v);
    uint64_t * d = (uint64_t*)&val;
    uint64_t one = 1;
    *d = ((*d >> one) << one) + (one);
    v->data = *d;
}

double matte_value_get_number(matteValue_t v) {
    uint64_t one = 1;
    uint64_t d = (v.data >> one) << one;    
    double * dd = (double*)&d;
    return *dd;
}   




static void isa_add(matteStore_t * store, matteArray_t * arr, matteValue_t * v) {
    matte_array_push(arr, v->value.id);
    // roll out all types
    MatteTypeData d = matte_array_at(store->typecode2data, MatteTypeData, v->value.id);
    if (d.isa) {
        uint32_t i;
        uint32_t count = matte_array_get_size(d.isa);
        for(i = 0; i < count; ++i) {
            matte_array_push(arr, matte_array_at(d.isa, uint32_t, i));
        }
    }
}

static matteArray_t * type_array_to_isa(matteStore_t * store, matteValue_t val) {
    uint32_t count = matte_value_object_get_key_count(store, val);
    uint32_t i;
    matteArray_t * array = matte_array_create(sizeof(uint32_t));

    if (count == 0) {
        matte_vm_raise_error_cstring(store->vm, "'inherits' attribute cannot be empty.");
        return array;
    }

    for(i = 0; i < count; ++i) {
        matteValue_t * v = matte_value_object_array_at_unsafe(store, val, i);
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_TYPE) {
            matte_vm_raise_error_cstring(store->vm, "'inherits' attribute must have Type values only.");
            return array;
        }
        isa_add(store, array, v);
    }
    return array;
}

static matteTable_t * type_array_get_layout(matteStore_t * store, matteValue_t val, matteValue_t layout) {
    matteTable_t * layoutOut = NULL;
    if (matte_value_type(layout) != MATTE_VALUE_TYPE_EMPTY) {
        if (matte_value_type(layout) != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(store->vm, "'layout' attribute must be an Object when present");
            return NULL;
        }
        layoutOut = matte_table_create_hash_pointer();

        matteObject_t * m = matte_store_bin_fetch_table(store->bin, layout.value.id);
        
        
        if (m->table.keyvalues_id) {

            matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
            matteArray_t * values = matte_array_create(sizeof(matteValue_t));
            
            matte_mvt2_get_all_keys(m->table.keyvalues_id, keys);
            matte_mvt2_get_all_values(m->table.keyvalues_id, values);

            uint32_t i;
            uint32_t len = matte_array_get_size(keys);

            for(i = 0; i < len; ++i) {
                matteValue_t key = matte_array_at(keys, matteValue_t, i);
                if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) continue;
                matteValue_t value = matte_array_at(values, matteValue_t, i);
                
                if (matte_value_type(value) != MATTE_VALUE_TYPE_TYPE) {                    
                    matteString_t * err = matte_string_create_from_c_str(
                        "'layout' attribute \"%s\" is not a Type.",
                        matte_string_get_c_str(matte_string_store_find(store->stringStore, key.value.id))
                    );
                    matte_vm_raise_error_string(store->vm, err);
                    matte_string_destroy(err);
                    matte_array_destroy(keys);
                    matte_array_destroy(values);
                    
                    matte_table_destroy(layoutOut);
                    return NULL;
                }
                
                uint32_t typecode = value.value.id;
                matte_table_insert_by_uint(
                    layoutOut,
                    key.value.id,
                    (void*)(uintptr_t)typecode
                );
            }
            matte_array_destroy(keys);
            matte_array_destroy(values);
        }
    }

    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {
        uint32_t count = matte_value_object_get_key_count(store, val);
        uint32_t i;
        matteArray_t * array = matte_array_create(sizeof(uint32_t));

        matteTableIter_t * iter = matte_table_iter_create();        

        for(i = 0; i < count; ++i) {
            matteValue_t * v = matte_value_object_array_at_unsafe(store, val, i);
            if (matte_value_type(*v) == MATTE_VALUE_TYPE_TYPE) {
                MatteTypeData d = matte_array_at(store->typecode2data, MatteTypeData, v->value.id);
                

                if (d.layout) {
                    if (layoutOut == NULL)
                        layoutOut = matte_table_create_hash_pointer();
                    
                    for(matte_table_iter_start(iter, d.layout);
                        matte_table_iter_is_end(iter) == 0;
                        matte_table_iter_proceed(iter)
                    ) {
                        
                        uint32_t key = matte_table_iter_get_key_uint(iter);                
                        uint32_t typecode = (uint32_t)(uintptr_t) matte_table_iter_get_value(iter);
                        uint32_t existing = (uint32_t)(uintptr_t)matte_table_find_by_uint(layoutOut, key);
                        
                        if (existing != 0 && existing != typecode) {
                            matteString_t * err = matte_string_create_from_c_str(
                                "'layout' attribute \"%s\" has multiple entries with differing types.",
                                matte_string_get_c_str(matte_string_store_find(store->stringStore, key))
                            );
                            matte_vm_raise_error_string(store->vm, err);
                            matte_string_destroy(err);

                            matte_table_iter_destroy(iter);
                            matte_table_destroy(layoutOut);
                            return NULL;
                        }
                        
                        matte_table_insert_by_uint(
                            layoutOut,
                            key,
                            (void*)(uintptr_t)typecode
                        );
                        
                    }
                }
            }
        }
        
        matte_table_iter_destroy(iter);
    }
    return layoutOut;
}

matteValue_t matte_value_create_type_(
    matteStore_t * store, 
    matteValue_t name, 
    matteValue_t inherits,
    matteValue_t layout
) {
    matteValue_t v = {};
    v.binIDreserved = MATTE_VALUE_TYPE_TYPE;
    if (store->typepool == 0xffffffff) {
        v.value.id = 0;
        matte_vm_raise_error_cstring(store->vm, "Type count limit reached. No more types can be created.");
    } else {
        v.value.id = ++(store->typepool);
    }
    MatteTypeData data = {};
    data.name = store->specialString_empty;

    matteValue_t val = name;
    if (matte_value_type(val)) {
        data.name = matte_value_as_string(store, val);
        matte_store_recycle(store, val);
    } else {
        matteString_t * str = matte_string_create_from_c_str("<anonymous type %d>", v.value.id);
        data.name.value.id = matte_string_store_ref(store->stringStore, str);
        data.name.binIDreserved = MATTE_VALUE_TYPE_STRING;
        matte_string_destroy(str);        
    }


    val = inherits;
    if (matte_value_type(val)) {
        if (matte_value_type(val) != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(store->vm, "'inherits' attribute must be an object.");
            matte_store_recycle(store, val);
            v.binIDreserved = 0;
            return v;
        }            
        
        data.isa = type_array_to_isa(store, val);
        matte_store_recycle(store, val);
    } 
    
    data.layout = type_array_get_layout(store, val, layout);
    matte_array_push(store->typecode2data, data);
    return v;
}


// Sets the type to a number with the given value
void matte_value_into_boolean_(matteStore_t * store, matteValue_t * v, int val) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_BOOLEAN;
    v->value.boolean = val;
}

void matte_value_into_string_(matteStore_t * store, matteValue_t * v, const matteString_t * str) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_STRING;
    v->value.id = matte_string_store_ref(store->stringStore, str);
}
matteValue_t matte_value_query(matteStore_t * store, matteValue_t * v, matteQuery_t query) {
    matteValue_t out = matte_store_new_value(store);
    switch(query) {
      case MATTE_QUERY__TYPE:
        return matte_value_get_type(store, *v);


      case MATTE_QUERY__COS: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("cos requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, cos(matte_value_as_number(store, *v)));
        return out;

      case MATTE_QUERY__SIN: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("sin requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, sin(matte_value_as_number(store, *v)));
        return out;
      

      case MATTE_QUERY__TAN: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("tan requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, tan(matte_value_as_number(store, *v)));
        return out;
      
      
      
      case MATTE_QUERY__ACOS:
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("acos requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, acos(matte_value_as_number(store, *v)));
        return out;
      

      case MATTE_QUERY__ASIN: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("asin requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, asin(matte_value_as_number(store, *v)));
        return out;
      

      case MATTE_QUERY__ATAN:
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("atan requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, atan(matte_value_as_number(store, *v)));
        return out;
      

      case MATTE_QUERY__ATAN2: 
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__ATAN2);
      
      case MATTE_QUERY__SQRT: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("sqrt requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, sqrt(matte_value_as_number(store, *v)));
        return out;

      case MATTE_QUERY__ABS: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("abs requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, fabs(matte_value_as_number(store, *v)));
        return out;
      


      case MATTE_QUERY__ISNAN: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("isNaN requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_boolean(store, &out, isnan(matte_value_as_number(store, *v)));
        return out;
      
      
      case MATTE_QUERY__FLOOR: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("floor requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, floor(matte_value_as_number(store, *v)));
        return out;


      case MATTE_QUERY__CEIL: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("ceil requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, ceil(matte_value_as_number(store, *v)));
        return out;

      case MATTE_QUERY__ROUND: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("round requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, round(matte_value_as_number(store, *v)));
        return out;

      case MATTE_QUERY__RADIANS: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("radian conversion requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, matte_value_as_number(store, *v)* (MATTE_PI / 180.0));
        return out;

      case MATTE_QUERY__DEGREES: 
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * str = matte_string_create_from_c_str("degree conversion requires base value to be a number.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, matte_value_as_number(store, *v)* (180.0 / MATTE_PI));
        return out;









      case MATTE_QUERY__REMOVECHAR: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("removeChar requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__REMOVECHAR);
      }
      case MATTE_QUERY__SUBSTR: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("substr requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SUBSTR);
      }
      case MATTE_QUERY__SPLIT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("split requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SPLIT);
      }
      case MATTE_QUERY__SCAN: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("scan requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SCAN);
      }
      case MATTE_QUERY__LENGTH: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("length requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matteValue_t len = matte_store_new_value(store);
        const matteString_t * strVal = matte_value_string_get_string_unsafe(store, *v);
        matte_value_into_number(store, &len, matte_string_get_length(strVal));   
        return len;     
      }
      case MATTE_QUERY__SEARCH: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("search requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SEARCH);
      }
      case MATTE_QUERY__SEARCH_ALL: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("searchAll requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SEARCH_ALL);
      }
      case MATTE_QUERY__CONTAINS: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("contains requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__CONTAINS);
      }

      case MATTE_QUERY__REPLACE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("replace requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__REPLACE);
      }
      case MATTE_QUERY__FORMAT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("format requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__FORMAT);
      }
      case MATTE_QUERY__COUNT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("count requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__COUNT);
      }
      case MATTE_QUERY__CHARCODEAT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("charCodeAt requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__CHARCODEAT);
      }
      case MATTE_QUERY__CHARAT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("charAt requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__CHARAT);
      }
      case MATTE_QUERY__SETCHARCODEAT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("setCharCodeAt requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SETCHARCODEAT);
      }
      case MATTE_QUERY__SETCHARAT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * str = matte_string_create_from_c_str("setCharAt requires base value to be a string.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SETCHARAT);
      }








      case MATTE_QUERY__KEYCOUNT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("keycount requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, matte_value_object_get_key_count(store, *v));
        return out;
      }

      case MATTE_QUERY__SIZE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("size requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matte_value_into_number(store, &out, matte_value_object_get_number_key_count(store, *v));
        return out;
      }

      case MATTE_QUERY__SETSIZE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("setSize requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SETSIZE);
      }

      case MATTE_QUERY__KEYS: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("keys requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return matte_value_object_keys(store, *v);
      }

      case MATTE_QUERY__VALUES: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("values requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return matte_value_object_values(store, *v);
      }

      case MATTE_QUERY__PUSH: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("push requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__PUSH);
      }

      case MATTE_QUERY__POP: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("pop requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matteValue_t key = {};
        uint32_t index = matte_value_object_get_number_key_count(store, *v) - 1;
        matte_value_into_number(store, &key, index);

        matteValue_t * at = matte_value_object_access_direct(store, *v, key, 1);
        if (at) {
            matte_value_object_remove_key(store, *v, key);
            return *at;            
        }

        return matte_store_new_value(store);      
      }
      
      case MATTE_QUERY__INSERT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("insert requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__INSERT);
      }

      case MATTE_QUERY__REMOVE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("remove requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__REMOVE);
      }

      case MATTE_QUERY__SETATTRIBUTES: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("setAttributes requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SETATTRIBUTES);
      }

      case MATTE_QUERY__ATTRIBUTES: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("attributes requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        matteValue_t out = matte_store_new_value(store);

        const matteValue_t * op = matte_value_object_get_attributes_unsafe(store, *v);
        if (op)
            matte_value_into_copy(store, &out, *op);

        return out;
      }
      
      case MATTE_QUERY__SORT: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("sort requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SORT);
      }
      
      case MATTE_QUERY__SUBSET: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("subset requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SUBSET);
      }

      case MATTE_QUERY__FILTER: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("filter requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__FILTER);
      }

      case MATTE_QUERY__FINDINDEX: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("findIndex requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__FINDINDEX);
      }
      
      case MATTE_QUERY__FINDINDEXCONDITION: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("findIndexCondition requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__FINDINDEXCONDITION);
      }      

      case MATTE_QUERY__ISA: {
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__ISA);
      }

      case MATTE_QUERY__MAP: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("map requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__MAP);
      }
      
      case MATTE_QUERY__ANY: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("any requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__ANY);
      }
      
      case MATTE_QUERY__FOREACH: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("foreach requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__FOREACH);
      }      
      
      case MATTE_QUERY__ALL: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("all requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__ALL);
      }
      
      case MATTE_QUERY__REDUCE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("reduce requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__REDUCE);
      }

      case MATTE_QUERY__SET_IS_INTERFACE: {
        if (matte_value_type(*v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v->value.id)) {
            matteString_t * str = matte_string_create_from_c_str("setIsInterface requires base value to be an object.");
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return out;
        }
        return *matte_vm_get_external_builtin_function_as_value(store->vm, MATTE_EXT_CALL__QUERY__SET_IS_INTERFACE);
      }




      
    }

    matteString_t * str = matte_string_create_from_c_str("VM error: unknown query operator.");
    matte_vm_raise_error_string(store->vm, str);
    matte_string_destroy(str);
    return matte_store_new_value(store);
}



void matte_value_into_new_object_ref_(matteStore_t * store, matteValue_t * v) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_table(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE;  
    matte_store_garbage_collect__add_to_color(store, d);  
    v->value.id = d->storeID;
    d->typecode = store->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
}





void matte_value_into_new_object_ref_typed_(matteStore_t * store, matteValue_t * v, matteValue_t typeobj) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_table(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE;  
    matte_store_garbage_collect__add_to_color(store, d);  
    v->value.id = d->storeID;
    if (matte_value_type(typeobj) != MATTE_VALUE_TYPE_TYPE) {        
        matteString_t * str = matte_string_create_from_c_str("Cannot instantiate object without a Type. (given value is of type %s)", matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, typeobj))));
        matte_vm_raise_error_string(store->vm, str);
        matte_string_destroy(str);
        d->typecode = store->type_object.value.id;
    } else {
        d->typecode = typeobj.value.id;
        if (matte_array_at(store->typecode2data, MatteTypeData, typeobj.value.id).layout) {
            ENABLE_STATE(d, OBJECT_STATE__HAS_LAYOUT);            
        }
    }
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);

}


void matte_value_into_new_object_literal_ref_(matteStore_t * store, matteValue_t * v, const matteArray_t * arr) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_table(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE; 
    matte_store_garbage_collect__add_to_color(store, d);  
    v->value.id = d->storeID;
    d->typecode = store->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
    matte_value_object_push_lock(store, *v);
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    
    for(i = 0; i < len; i+=2) {
        matteValue_t key   = matte_array_at(arr, matteValue_t, i);
        matteValue_t value = matte_array_at(arr, matteValue_t, i+1);
        
        object_put_prop(store, d, key, value);
    }

    matte_value_object_pop_lock(store, *v);


}


void matte_value_into_new_object_array_ref_(matteStore_t * store, matteValue_t * v, const matteArray_t * args) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_table(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE;  
    matte_store_garbage_collect__add_to_color(store, d);  
    v->value.id = d->storeID;
    d->typecode = store->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
    matte_value_object_push_lock(store, *v);

    uint32_t i;
    uint32_t len = matte_array_get_size(args);

    if (!d->table.keyvalues_number) d->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));//(matteArray_t*)matte_pool_fetch(store->keyvalues_numberPool);
    matte_array_set_size(d->table.keyvalues_number, len);
    for(i = 0; i < len; ++i) {
        matteValue_t val = matte_store_new_value(store);
        matte_value_into_copy(store, &val, matte_array_at(args, matteValue_t, i));
        matte_array_at(d->table.keyvalues_number, matteValue_t, i) = val;
        if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {
            object_link_parent_value(store, d, &val);
        }
    }

    matte_value_object_pop_lock(store, *v);

}

matteValue_t matte_value_frame_get_named_referrable(matteStore_t * store, matteVMStackFrame_t * vframe, matteValue_t name) {
    if (!(IS_FUNCTION_ID(vframe->context.value.id))) return matte_store_new_value(store);
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, vframe->context.value.id);
    if (!m->function.stub) return matte_store_new_value(store);
    // claim captures immediately.
    uint32_t i;
    uint32_t len;



    // referrables come from a history of creation contexts.
    matteValue_t context = vframe->context;
    uint32_t originID = context.value.id;
    matteValue_t out = matte_store_new_value(store);

    while(originID) {
        #if MATTE_DEBUG__STORE
            printf("Looking in args..\n");
        #endif
        matteObject_t * origin = matte_store_bin_fetch_function(store->bin, originID);


        len = matte_bytecode_stub_arg_count(origin->function.stub);
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__STORE
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_bytecode_stub_get_arg_name_noref(origin->function.stub, i))),
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, name))
                );
            #endif
            matteValue_t argName = matte_bytecode_stub_get_arg_name_noref(origin->function.stub, i);
            if (argName.value.id == name.value.id) {
                matte_value_into_copy(store, &out, origin->function.vars->referrables[i]);
                return out;
            }
        }
        
        len = matte_bytecode_stub_local_count(origin->function.stub);
        #if MATTE_DEBUG__STORE
            printf("Looking in locals..\n");
        #endif
        for(i = 0; i < len; ++i) {
            #if MATTE_DEBUG__STORE
                printf("TESTING %s against %s\n",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_bytecode_stub_get_local_name_noref(origin->function.stub, i))),
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, name))
                );
            #endif
            if (matte_bytecode_stub_get_local_name_noref(origin->function.stub, i).value.id == name.value.id) {
                matte_value_into_copy(store, &out, origin->function.vars->referrables[i+matte_bytecode_stub_arg_count(origin->function.stub)]);
                return out;
            }
        }

        originID = origin->function.origin;
    }

    matte_vm_raise_error_cstring(store->vm, "Could not find named referrable!");
    return out;
}


void matte_value_into_new_typed_function_ref_(matteStore_t * store, matteValue_t * v, matteBytecodeStub_t * stub, const matteArray_t * args) {
    uint32_t i;
    uint32_t len = matte_array_get_size(args);
    // first check to see if the types are actually types.
    for(i = 0; i < len; ++i) {
        if (matte_value_type(matte_array_at(args, matteValue_t, i)) != MATTE_VALUE_TYPE_TYPE) {
            matteString_t * str;
            if (i == len-1) {
                str = matte_string_create_from_c_str("Function constructor with type specifiers requires those specifications to be Types. The return value specifier is not a Type.");
            } else {
                str = matte_string_create_from_c_str("Function constructor with type specifiers requires those specifications to be Types. The type specifier for Argument '%s' is not a Type.\n",                 matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_bytecode_stub_get_arg_name_noref(stub, i))));
            }
            
            matte_vm_raise_error_string(store->vm, str);
            matte_string_destroy(str);
            return;
        }
    }
    
    matte_value_into_new_function_ref(store, v, stub);
    matteObject_t * d = matte_store_bin_fetch_function(store->bin, v->value.id);
    d->function.types = matte_array_clone(args);
}


int matte_value_object_function_was_activated(matteStore_t * store, matteValue_t v) {
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    return m->function.referrablesCount != 0;
}

const matteValue_t ** matte_value_object_function_activate_closure(
    matteStore_t * store, 
    matteValue_t v, 
    matteValue_t * refs
) {
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    uint32_t i;

    m->function.vars->referrables = refs;    
    matteBytecodeStub_t * stub = m->function.stub;
        
    m->function.referrablesCount = (
        matte_bytecode_stub_arg_count(stub) + 
        matte_bytecode_stub_local_count(stub)
    );
    
    uint32_t len = m->function.referrablesCount;

    for(i = 0; i < len; ++i) {
        matteValue_t vSrc = refs[i];
        matteValue_t vv = matte_store_new_value(store);
        matte_value_into_copy(store, &vv, vSrc);
        if (matte_value_type(vv) == MATTE_VALUE_TYPE_OBJECT) {
            object_link_parent_value(store, m, &vv);
        }
    }
    return (const matteValue_t **)m->function.vars->captures;
}


void matte_value_into_cloned_function_ref_(matteStore_t * store, matteValue_t * v, matteValue_t source) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_function(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE;  
    matte_store_garbage_collect__add_to_color(store, d);  


    v->value.id = d->storeID;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
    
    
    
    /* // problem?
    if (external) {
        matte_value_object_push_lock(store, *v);
        matte_array_push(store->external, *v);
    }
    */
    
    matteObject_t * src = matte_store_bin_fetch_function(store->bin, source.value.id);
    
    d->function.stub = src->function.stub;
    d->function.origin = src->function.origin;
    if (d->function.origin)
        object_link_parent(store, d, matte_store_bin_fetch(store->bin, d->function.origin));

    if (src->function.types) {
        d->function.types = matte_array_clone(src->function.types);
    }

    d->function.isCloned = 1;
    d->function.vars->captures = src->function.vars->captures;
    d->function.vars->captureOrigins = src->function.vars->captureOrigins;

    // better? worse? you decide!
    object_link_parent(store, d, src);
}

void matte_value_object_function_set_closure_value_unsafe(matteStore_t * store, matteValue_t v, uint32_t index, matteValue_t val) {
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    #ifdef MATTE_DEBUG__STORE
        assert(index < m->function.referrablesCount);
    #endif
    
    matteValue_t vOld = m->function.vars->referrables[index];

    matteValue_t vNew = matte_store_new_value(store);
    matte_value_into_copy(store, &vNew, val);

    if (matte_value_type(vOld) == MATTE_VALUE_TYPE_OBJECT) {
        object_unlink_parent_value(store, m, &vOld);
    }
    
    matte_store_recycle(store, vOld);
    
    if (matte_value_type(vNew) == MATTE_VALUE_TYPE_OBJECT) {
        object_link_parent_value(store, m, &vNew);
    }
    m->function.vars->referrables[index] = vNew;
}



static void matte_value_into_new_function_ref_real(matteStore_t * store, matteValue_t * v, matteBytecodeStub_t * stub, int external) {
    matte_store_recycle(store, *v);
    v->binIDreserved = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_store_bin_add_function(store->bin);
    #ifdef MATTE_DEBUG__STORE
        assert(d->prevColor == d->nextColor && d->prevColor == 0);
    #endif
    d->color = OBJECT_TRICOLOR__WHITE;  
    matte_store_garbage_collect__add_to_color(store, d);  


    v->value.id = d->storeID;
    d->function.stub = stub;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);

    if (external) {
        matte_value_object_push_lock(store, *v);
        matte_array_push(store->external, *v);
    }



    // claim captures immediately.
    uint32_t i;
    uint32_t len;
    const matteBytecodeStubCapture_t * capturesRaw = matte_bytecode_stub_get_captures(
        stub,
        &len
    );
    matteVMStackFrame_t frame = {};

    // save origin so that others may use referrables.
    // This happens in every case EXCEPT the 0-stub function.
    if (matte_vm_get_stackframe_size(store->vm)) {
        frame = matte_vm_get_stackframe(store->vm, 0);
        matteObject_t * origin = matte_store_bin_fetch_function(store->bin, frame.context.value.id);
        d->function.origin = origin->storeID;
        object_link_parent(store, d, matte_store_bin_fetch(store->bin, d->function.origin));
    }
    // TODO: can we speed this up?
    if (!len) return;


    // referrables come from a history of creation contexts.
    matteVariableData_t * vars = d->function.vars;
    vars->captures = (matteValue_t**)matte_allocate(len * sizeof(matteValue_t *));
    vars->captureOrigins = (uint32_t*)matte_allocate(len * sizeof(uint32_t));
    
    for(i = 0; i < len; ++i) {
        matteValue_t context = frame.context;

        matteObject_t * origin = matte_store_bin_fetch_function(store->bin, context.value.id);
        while(origin) {
            if (matte_bytecode_stub_get_id(origin->function.stub) == capturesRaw[i].stubID) {
                object_link_parent(store, d, origin);
                vars->captures[i] = origin->function.vars->referrables+capturesRaw[i].referrable;
                vars->captureOrigins[i] = origin->storeID;
                break;
            }
            origin = matte_store_bin_fetch(store->bin, origin->function.origin);
        }

        if (!origin) {
            matte_vm_raise_error_cstring(store->vm, "Could not find captured variable!");
            static matteValue_t err = {};
            vars->captures[i] = &err;
        }
    }
    
    d->function.isCloned = 0;
}

void matte_value_into_new_function_ref_(matteStore_t * store, matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_value_into_new_function_ref_real(
        store, v, stub, 0
    );
    
}
void matte_value_into_new_external_function_ref_(matteStore_t * store, matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_value_into_new_function_ref_real(
        store, v, stub, 1
    );
}




matteValue_t * matte_value_object_array_at_unsafe(matteStore_t * store, matteValue_t v, uint32_t index) {
    matteObject_t * d = matte_store_bin_fetch_table(store->bin, v.value.id);
    return &matte_array_at(d->table.keyvalues_number, matteValue_t, index);
}

void matte_value_object_array_set_size_unsafe(matteStore_t * store, matteValue_t v, uint32_t size) {
    matteObject_t * d = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (!d->table.keyvalues_number) d->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));//(matteArray_t*)matte_pool_fetch(store->keyvalues_numberPool);
    matteArray_t * arr = d->table.keyvalues_number;
    uint32_t i;
    uint32_t oldSize = matte_array_get_size(arr);
    if (size > matte_array_get_size(arr)) {
        matte_array_set_size(arr, size);
        for(i = oldSize; i < size; ++i) {
            matte_array_at(arr, matteValue_t, i) = matte_store_new_value(store);
        }
    } else if (size == oldSize) {
        // whyd you do that!
    } else {
        for(i = size; i < oldSize; ++i) {
            matteValue_t v = matte_array_at(arr, matteValue_t, i);
            if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, d, &v);                
            }
            matte_store_recycle(store, v);
        }
        matte_array_set_size(arr, size);
    }
}


const matteString_t * matte_value_string_get_string_unsafe(matteStore_t * store, matteValue_t v) {
    #ifdef MATTE_DEBUG 
    assert(matte_value_type(v) == MATTE_VALUE_TYPE_STRING);
    #endif
    return matte_string_store_find(store->stringStore, v.value.id);
}


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT && IS_FUNCTION_ID(v.value.id)) {
        matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
        return m->function.stub;
    }
    return NULL;
}

matteValue_t * matte_value_get_captured_value(matteStore_t * store, matteValue_t v, uint32_t index) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT && IS_FUNCTION_ID(v.value.id)) {
        matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
        return m->function.vars->captures[index];
    }
    return NULL;
}

void matte_value_set_captured_value(matteStore_t * store, matteValue_t v, uint32_t index, matteValue_t val) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT && IS_FUNCTION_ID(v.value.id)) {
        matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);

        matteObject_t * origin = matte_store_bin_fetch_function(store->bin, 
            m->function.vars->captureOrigins[index]
        );
        
        matteValue_t vNew = matte_store_new_value(store);
        matte_value_into_copy(store, &vNew, val);        
        if (matte_value_type(*m->function.vars->captures[index]) == MATTE_VALUE_TYPE_OBJECT) {        
            object_unlink_parent_value(store, origin, m->function.vars->captures[index]);
        }
        
        matte_store_recycle(store, *m->function.vars->captures[index]);        

        if (matte_value_type(vNew) == MATTE_VALUE_TYPE_OBJECT) {
            object_link_parent_value(store, origin, &vNew);
        }    
        
        *m->function.vars->captures[index] = vNew;
        



        /*
        if (index >= m->function.capturesCount) return;

        CapturedReferrable_t referrable = m->function.captures[index];
        matteObject_t * functionSrc = matte_store_bin_fetch_function(store->bin, referrable.functionID);
        matteValue_t vOld = matte_store_bin_fetch_function(store->bin, referrable.functionID)->function.referrables[referrable.index];

        matteValue_t vNew = matte_store_new_value(store);
        matte_value_into_copy(store, &vNew, val);

        if (matte_value_type(vOld) == MATTE_VALUE_TYPE_OBJECT) {
            object_unlink_parent_value(store, functionSrc, &vOld);
        }        
        matte_store_recycle(store, vOld);
        
        if (matte_value_type(vNew) == MATTE_VALUE_TYPE_OBJECT) {
            object_link_parent_value(store, functionSrc, &vNew);
        }    


        functionSrc->function.referrables[referrable.index] = vNew;
        */
    }
}

#ifdef MATTE_DEBUG


static void print_object_children__print_value(const char * role, matteValue_t v) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT)
        printf(" - %s: %d\n", role, v.value.id);
    else 
        printf(" - %s: [plain value] \n", role, v.value.id);
}
static void print_object_children(matteStore_t * h, matteObject_t * o) {
    matteTableIter_t * iter = matte_table_iter_create();        

    uint32_t i;
    uint32_t n;
    
    if (IS_FUNCTION_OBJECT(o)) {

        uint32_t subl;
        matte_bytecode_stub_get_captures(o->function.stub, &subl);
        // should be unlinked already because of above
        for(n = 0; n < subl; ++n) {
            matteValue_t dummy = {};
            dummy.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
            dummy.value.id = o->storeID;
            print_object_children__print_value("function capture", *matte_value_get_captured_value(h, dummy, n));
        }
    } else {
        if (o->table.attribSet && matte_value_type(*o->table.attribSet)) {
            print_object_children__print_value("attrib set", *o->table.attribSet);
        }


        if (o->table.keyvalues_number && matte_array_get_size(o->table.keyvalues_number)) {
            uint32_t subl = matte_array_get_size(o->table.keyvalues_number);
            for(n = 0; n < subl; ++n) {
                print_object_children__print_value("key [number]", matte_array_at(o->table.keyvalues_number, matteValue_t, n));                
            }
        }

        if (o->table.keyvalues_id && matte_mvt2_get_size(o->table.keyvalues_id)) {
            matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
            matteArray_t * values = matte_array_create(sizeof(matteValue_t));
            
            matte_mvt2_get_all_keys(o->table.keyvalues_id, keys);
            matte_mvt2_get_all_values(o->table.keyvalues_id, values);

            uint32_t i;
            uint32_t len = matte_array_get_size(keys);

            for(i = 0; i < len; ++i) {
                matteValue_t id = matte_array_at(keys, matteValue_t, i);
                matteValue_t value = matte_array_at(values, matteValue_t, i);
                switch (matte_value_type(id)) {
                  case MATTE_VALUE_TYPE_STRING:
                    matteString_t * role = matte_string_create_from_c_str("key [string %d -> %s]", 
                        (int)id.value.id,
                        matte_string_get_c_str(matte_string_store_find(h->stringStore, id.value.id))
                    );
                    print_object_children__print_value(matte_string_get_c_str(role), value);
                    matte_string_destroy(role);
                    break;
                    
                  case MATTE_VALUE_TYPE_TYPE:
                    print_object_children__print_value("key [type]", value);
                    break;
                    
                  case MATTE_VALUE_TYPE_OBJECT:
                    print_object_children__print_value("key [object]", value);
                    break;

                }
            }
            matte_array_destroy(keys);
            matte_array_destroy(values);
        }
    }
    matte_table_iter_destroy(iter);

}


void matte_value_print(matteStore_t * store, matteValue_t v) {
    printf(    "Matte STORE Object:\n");
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_EMPTY: 
        printf("  Store Type: EMPTY\n");
        break;
      case MATTE_VALUE_TYPE_BOOLEAN: 
        printf("  Store Type: BOOLEAN\n");
        printf("  Value:     %s\n", matte_value_as_boolean(store, v) ? "true" : "false");
        break;
      case MATTE_VALUE_TYPE_NUMBER: 
        printf("  Store Type: NUMBER\n");
        printf("  Value:     %f\n", matte_value_as_number(store, v));
        break;
      case MATTE_VALUE_TYPE_STRING: 
        printf("  Store Type: STRING\n");
        printf("  String ID:   %d\n", v.value.id);
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(store, v)));
        break;
      case MATTE_VALUE_TYPE_TYPE: 
        printf("  Store Type: TYPE\n");
        printf("  Value:     %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_as_string(store, v))));
        break;    
      case MATTE_VALUE_TYPE_OBJECT: {
        printf("  Store Type: OBJECT %s\n", IS_FUNCTION_ID(v.value.id) ? "(Function)" : "(table)");
        printf("  Store ID:   %d\n", v.value.id);
        matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
        if (!m) {
            printf("  (WARNING: this refers to an object fully recycled.)\n");
            fflush(stdout);
            return;
        }
        printf("  color  :   %s\n", m->color == OBJECT_TRICOLOR__BLACK ? "black" : m->color == OBJECT_TRICOLOR__GREY ? "grey":"white");
        printf("         %d<->%d\n", m->prevColor ? m->prevColor : -1, m->nextColor ? m->nextColor : -1);

        //printf("  refct  :   %d\n", (int)m->refcount);

        if (QUERY_STATE(m, OBJECT_STATE__RECYCLED)) {
            printf("  (WARNING: this object is currently dormant.)\n") ;          
        }
        printf("  RootLock:  %d\n", m->rootState);
        printf("  State   :  %d\n", m->state);
        #ifdef MATTE_DEBUG__STORE
            printf("  Parents : "); 
            uint32_t i;
            for(i = 0; i < matte_array_get_size(m->parents); ++i) {
                printf("%d ", matte_array_at(m->parents, matteValue_t, i).value.id);
            }
            printf("\n");
        #endif
        
        printf("  Children:\n");
        print_object_children(store, m);
        

        //printf("  Parents?   %s\n", !matte_array_get_size(m->refParents) ? "false" : "true");
        //matte_store_object_print_parents(store, m->storeID);
        //printf("  Children?  %s\n", !matte_array_get_size(m->refChildren) ? "false" : "true");
        //matte_store_object_print_children(store, m->storeID);
        /*{
            printf("  String Values:\n");
            matteObject_t * m = matte_bin_fetch(store->sortedStores[MATTE_VALUE_TYPE_OBJECT], v.value.id);            
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


void matte_value_print_from_id(matteStore_t * store, uint32_t bin, uint32_t v) {
    matteValue_t val;
    val.binIDreserved = bin;
    val.value.id = v;

    matte_value_print(store, val);
}
#endif

void matte_value_object_set_native_finalizer(matteStore_t * store, matteValue_t v, void (*fb)(void * objectUserdata, void * functionUserdata), void * functionUserdata) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Tried to set native finalizer on a non-object value.");
        return;
    }
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
    if (!m->ext) m->ext = (matteObjectExternalData_t*)matte_allocate(sizeof(matteObjectExternalData_t));
    m->ext->nativeFinalizer = fb;
    m->ext->nativeFinalizerData = functionUserdata;        
}


void matte_value_object_set_userdata(matteStore_t * store, matteValue_t v, void * userData) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
        if (!m->ext) m->ext = (matteObjectExternalData_t*)matte_allocate(sizeof(matteObjectExternalData_t));
        m->ext->userdata = userData;
    }
}

void * matte_value_object_get_userdata(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
        if (!m->ext) return NULL;
        return m->ext->userdata;
    }
    return NULL;
}



double matte_value_as_number(matteStore_t * store, matteValue_t v) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_EMPTY: 
        matte_vm_raise_error_cstring(store->vm, "Cannot convert empty into a number.");
        return 0;
      case MATTE_VALUE_TYPE_TYPE: 
        matte_vm_raise_error_cstring(store->vm, "Cannot convert type value into a number.");
        return 0;

      case MATTE_VALUE_TYPE_NUMBER: {
        return matte_value_get_number(v);
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        return v.value.boolean;
      }
      case MATTE_VALUE_TYPE_STRING: {
        matte_vm_raise_error_cstring(store->vm, "Cannot convert string value into a number.");
        return 0;
      }

      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id)) {
            matte_vm_raise_error_cstring(store->vm, "Cannot convert function value into a number.");
            return 0;        
        }
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        matteValue_t operator_ = object_get_conv_operator(store, m, store->type_number.value.id);
        if (matte_value_type(operator_)) {            
            double num = matte_value_as_number(store, matte_vm_call(store->vm, operator_, matte_array_empty(), matte_array_empty(), NULL));
            matte_store_recycle(store, operator_);
            return num;
        } else {
            matte_vm_raise_error_cstring(store->vm, "Cannot convert object into a number. No 'Number' property in 'operator' object.");
        }
      }
    }
    return 0;
}

matteValue_t matte_value_as_string(matteStore_t * store, matteValue_t v) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_EMPTY: { 
        //matte_vm_raise_error_cstring(store->vm, "Cannot convert empty into a string.");
        ///return 0;
        matteValue_t out;
        matte_value_into_copy(store, &out, store->specialString_empty);
        return out;
      }
      case MATTE_VALUE_TYPE_TYPE: { 
        matteValue_t out;
        matte_value_into_copy(store, &out, matte_value_type_name_noref(store, v));
        return out;
      }

      // slow.. make fast please!
      case MATTE_VALUE_TYPE_NUMBER: {        
        matteString_t * str;
        double val = matte_value_get_number(v);
        if (fabs(val - (int64_t)val) < DBL_EPSILON) {
            str = matte_string_create_from_c_str("%" PRId64 "", (int64_t)val);                
        } else {
            str = matte_string_create_from_c_str("%.15g", val);        
        }
        matteValue_t out;
        out.binIDreserved = MATTE_VALUE_TYPE_STRING;
        out.value.id = matte_string_store_ref(store->stringStore, str);
        matte_string_destroy(str);        
        return out;        
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        matteValue_t out;
        matte_value_into_copy(store, &out, v.value.boolean ? store->specialString_true : store->specialString_false);
        return out;
      }
      case MATTE_VALUE_TYPE_STRING: {
        matteValue_t out = matte_store_new_value(store);
        matte_value_into_copy(store, &out, v);
        return out;
      }
      


      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id)) {
            matte_vm_raise_error_cstring(store->vm, "Cannot convert function into a string.");
            matteValue_t out;
            matte_value_into_copy(store, &out, store->specialString_empty);
            return out;        
        }
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        matteValue_t operator_ = object_get_conv_operator(store, m, store->type_string.value.id);
        if (matte_value_type(operator_)) {
            matteValue_t result = matte_vm_call(store->vm, operator_, matte_array_empty(), matte_array_empty(), NULL);
            matte_store_recycle(store, operator_);
            matteValue_t f = matte_value_as_string(store, result);
            matte_store_recycle(store, result);
            return f;
        } else {
            matte_vm_raise_error_cstring(store->vm, "Object has no valid conversion to string.");
        }
      }
    }
    matteValue_t out;
    matte_value_into_copy(store, &out, store->specialString_empty);
    return out;        
}


matteValue_t matte_value_to_type(matteStore_t * store, matteValue_t v, matteValue_t t) {
    matteValue_t out = matte_store_new_value(store);
    switch(t.value.id) {
      case 1: { // empty
        if (matte_value_type(v) != MATTE_VALUE_TYPE_EMPTY) {
            matte_vm_raise_error_cstring(store->vm, "It is an error to convert any non-empty value to the Empty type.");
        } else {
            matte_value_into_copy(store, &out, v);
        }

        break;
      }

      case 7: {
        if (matte_value_type(v) != MATTE_VALUE_TYPE_TYPE) {
            matte_vm_raise_error_cstring(store->vm, "It is an error to convert any non-Type value to a Type.");
        } else {
            matte_value_into_copy(store, &out, v);
        }
        break;
      }

      
      case 2: {
        matte_value_into_boolean(store, &out, matte_value_as_boolean(store, v));
        break;
      }
      case 3: {
        matte_value_into_number(store, &out, matte_value_as_number(store, v));
        break;
      }


      case 4: {
        out = matte_value_as_string(store, v);
        break;
      }



      case 5: {
        if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT && !IS_FUNCTION_ID(v.value.id)) {
            matte_value_into_copy(store, &out, v);
            break;
        } else {
            matte_vm_raise_error_cstring(store->vm, "Cannot convert value to Object type.");        
        }
      
      }
      
      case 6: {
        if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT && IS_FUNCTION_ID(v.value.id)) {
            matte_value_into_copy(store, &out, v);
            break;
        } else {
            matte_vm_raise_error_cstring(store->vm, "Cannot convert value to Function type.");        
        }
      }
      
      
      default:;// TODO: custom types.
      
      
    
    }
    return out;
}

int matte_value_as_boolean(matteStore_t * store, matteValue_t v) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_TYPE: {
        return 1;
      }
      case MATTE_VALUE_TYPE_NUMBER: {
        return matte_value_get_number(v) != 0.0;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        return v.value.boolean;
      }
      case MATTE_VALUE_TYPE_STRING: {
        // non-empty value
        return 1;
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id))
            return 1;              
            
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        matteValue_t operator_ = object_get_conv_operator(store, m, store->type_boolean.value.id);
        if (matte_value_type(operator_)) {
            matteValue_t r = matte_vm_call(store->vm, operator_, matte_array_empty(), matte_array_empty(), NULL);
            int out = matte_value_as_boolean(store, r);
            matte_store_recycle(store, operator_);
            matte_store_recycle(store, r);
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
int matte_value_is_callable(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_TYPE) return 1;
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) return 0;
    if (!IS_FUNCTION_ID(v.value.id)) return 0;
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    return 1 + (m->function.types != NULL); 
}

int matte_value_object_function_pre_typecheck_unsafe(matteStore_t * store, matteValue_t v, const matteArray_t * arr) {
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);

    #ifdef MATTE_DEBUG__STORE
        assert(len == matte_array_get_size(m->function.types) - 1);    
    #endif    

    for(i = 0; i < len; ++i) {
        if (!matte_value_isa(
            store, 
            matte_array_at(arr, matteValue_t, i),
            matte_array_at(m->function.types, matteValue_t, i)
        )) {
            matteString_t * err = matte_string_create_from_c_str(
                "Argument '%s' (type: %s) is not of required type %s",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_bytecode_stub_get_arg_name_noref(m->function.stub, i))),
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, matte_array_at(arr, matteValue_t, i))))),
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_array_at(m->function.types, matteValue_t, i))))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return 0;
        }
    }
    
    return 1;
}

void matte_value_object_function_post_typecheck_unsafe(matteStore_t * store, matteValue_t v, matteValue_t ret) {
    matteObject_t * m = matte_store_bin_fetch_function(store->bin, v.value.id);
    if (!matte_value_isa(store, ret, matte_array_at(m->function.types, matteValue_t, matte_array_get_size(m->function.types) - 1))) {
        matteString_t * err = matte_string_create_from_c_str(
            "Return value (type: %s) is not of required type %s.",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, ret)))),
            matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_array_at(m->function.types, matteValue_t, matte_array_get_size(m->function.types) - 1))))

        );
        matte_vm_raise_error_string(store->vm, err);
        matte_string_destroy(err);
    }  
}




// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteStore_t * store, matteValue_t v, matteValue_t key, int isBracketAccess) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_TYPE: {// built-in functions
        matteValue_t * b = matte_value_object_access_direct(store, v, key, isBracketAccess);
        if (b) {
            matteValue_t out = matte_store_new_value(store);
            matte_value_into_copy(store, &out, *b);
            return out;        
        }
        return matte_store_new_value(store);        
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id)) {
            matteString_t * err = matte_string_create_from_c_str(
                "Functions do not have members."
            );
            matte_vm_raise_error_string(store->vm, err);
            return matte_store_new_value(store);        
        }
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        
        matteValue_t accessor = {};
        
        int hasInterface = QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE);
        
        if (hasInterface == 0 || (hasInterface && isBracketAccess))
            accessor = object_get_access_operator(store, m, isBracketAccess, 1);
        
                
        if (hasInterface && matte_value_type(accessor) == 0) {
            if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) {
                matte_vm_raise_error_cstring(store->vm, "Objects with interfaces only have string-keyed members.");
                return matte_store_new_value(store);          
            }
            
            if (!m->table.keyvalues_id) {
                matte_vm_raise_error_cstring(store->vm, "Object's interface was empty.");
                return matte_store_new_value(store);          
            }        
            
            matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);
            if (value == NULL) {
                matteString_t * err = matte_string_create_from_c_str(
                    "Object's interface has no member \"%s\".",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
                );
                matte_vm_raise_error_string(store->vm, err);
                matte_string_destroy(err);
                return matte_store_new_value(store);               
            }
            if (matte_value_type(*value) != MATTE_VALUE_TYPE_OBJECT) {
                matteString_t * err = matte_string_create_from_c_str(
                    "Object's interface member \"%s\" is neither a Function nor an Object. Interface is malformed.",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
                );
                matte_vm_raise_error_string(store->vm, err);
                matte_string_destroy(err);
                return matte_store_new_value(store);                
            
            }            
            if (IS_FUNCTION_ID(value->value.id)) {
                matteValue_t vv = matte_store_new_value(store);
                matte_value_into_copy(store, &vv, *value);
                return vv;
            }
            
            matteObject_t * setget = matte_store_bin_fetch_table(store->bin, value->value.id);
            if (setget->table.keyvalues_id == NULL) {
                return matte_store_new_value(store);               
            }
            matteValue_t * getter = matte_mvt2_find(setget->table.keyvalues_id, store->specialString_get);
            if (getter == NULL) {
                matteString_t * err = matte_string_create_from_c_str(
                    "Object's interface disallows reading of the member \"%s\".",
                    matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
                );
                matte_vm_raise_error_string(store->vm, err);
                matte_string_destroy(err);
                return matte_store_new_value(store);               
            }

            matteValue_t privateBinding = {};
            if (m->table.privateBinding) {
                privateBinding = *m->table.privateBinding;
            }
            
            return matte_vm_call_full(store->vm, *getter, privateBinding, matte_array_empty(), matte_array_empty(), NULL);
        }        
        
        
        
        
        if (matte_value_type(accessor)) {
            matteValue_t args[1] = {
                key
            };
            matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 1);
            matteArray_t arrName = MATTE_ARRAY_CAST(&store->specialString_key, matteValue_t, 1);

            matteValue_t a = matte_vm_call(store->vm, accessor, &arr, &arrName, NULL);
            matte_store_recycle(store, accessor);
            return a;
        } else {
            matteValue_t vv = matte_store_new_value(store);
            matteValue_t * vvp = object_lookup(store, m, key);
            if (vvp) {
                matte_value_into_copy(store, &vv, *vvp);
            }
            return vv;
        }
      
      }
      
      default: {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, v))))
        );
        matte_vm_raise_error_string(store->vm, err);
        return matte_store_new_value(store);
      }
      
    }


}


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t * matte_value_object_access_direct(matteStore_t * store, matteValue_t v, matteValue_t key, int isBracketAccess) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_TYPE: {
        if (isBracketAccess) {
            matteString_t * err = matte_string_create_from_c_str(
                "Types can only yield access to built-in functions through the dot '.' accessor."
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL;
        }
        if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * err = matte_string_create_from_c_str(
                "Types can only yield access to built-in functions through the string keys."
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL;
        }
        
        matteTable_t * base = NULL;
        switch(v.value.id) {
          case 3: base = store->type_number_methods; break;            
          case 4: base = store->type_string_methods; break;
          case 5: base = store->type_object_methods; break;
          default:;
        };
        
        if (!base) {
            matteString_t * err = matte_string_create_from_c_str(
                "The given type has no built-in functions. (Given value is of type: %s)",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, v))))
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL;
        }
        
        int out = (uintptr_t)matte_table_find_by_uint(base, key.value.id);
        // for Types, its an error if no function is found.
        if (!out) {
            matteString_t * err = matte_string_create_from_c_str(
                "The given type has no built-in function by the name '%s'",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return NULL;
        }
        return matte_vm_get_external_builtin_function_as_value(store->vm, (matteExtCall_t)out);
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id)) {
            matteString_t * err = matte_string_create_from_c_str(
                "Cannot access member of type Function (Functions do not have members).",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, v))))
            );
            matte_vm_raise_error_string(store->vm, err);
            return NULL; //matte_store_new_value(store);        
        }

        
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        if (QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE)) return NULL;
        matteValue_t accessor = object_get_access_operator(store, m, isBracketAccess, 1);
        if (matte_value_type(accessor)) {
            return NULL;
        } else {
            return object_lookup(store, m, key);
        }      
      }

      default: {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(store, matte_value_type_name_noref(store, matte_value_get_type(store, v))))
        );
        matte_vm_raise_error_string(store->vm, err);
        return NULL; //matte_store_new_value(store);
      
      
      }
        
    }

}



matteValue_t matte_value_object_access_string(matteStore_t * store, matteValue_t v, const matteString_t * key) {
    matteValue_t keyO = matte_store_new_value(store);
    matte_value_into_string(store, &keyO, key);
    matteValue_t result = matte_value_object_access(store, v, keyO, 0);
    matte_store_recycle(store, keyO);
    return result;
}

matteValue_t matte_value_object_access_index(matteStore_t * store, matteValue_t v, uint32_t index) {
    matteValue_t keyO = matte_store_new_value(store);
    matte_value_into_number(store, &keyO, index);
    matteValue_t result = matte_value_object_access(store, v, keyO, 1);
    matte_store_recycle(store, keyO);
    return result;
}



void matte_value_object_push_lock_(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
    if (m->rootState == 0) {

        uint32_t n = matte_pool_add(store->nodes);
        matteObjectNode_t * node = matte_pool_fetch(store->nodes, matteObjectNode_t, n);
        node->data = m->storeID;
        if (store->roots) {
            node->next = store->roots;
        }
        store->roots = n;

        if (m->color == OBJECT_TRICOLOR__WHITE) {
            matte_store_garbage_collect__rem_from_color(store, m);
            m->color = OBJECT_TRICOLOR__GREY; // not accurate, but lets the algorithm work correctly
            matte_store_garbage_collect__add_to_color(store, m);
        }
    }
    m->rootState++;
    #ifdef MATTE_DEBUG__STORE_LEVEL_2   
    printf("%d + at state %d\n", v.value.id, m->rootState);
    #endif
}

void matte_value_object_pop_lock_(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
    if (m->rootState) {
        m->rootState--;
        if (m->rootState == 0) {
            if (m->color == OBJECT_TRICOLOR__BLACK) {
                matte_store_garbage_collect__rem_from_color(store, m);
                m->color = OBJECT_TRICOLOR__GREY;
                matte_store_garbage_collect__add_to_color(store, m);
            }
            store->gcRequestStrength++;
        }
    }     
    #ifdef MATTE_DEBUG__STORE_LEVEL_2
    printf("%d - at state %d\n", v.value.id, m->rootState);
    #endif
}



matteValue_t matte_value_object_keys(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "Can only get keys from something that's an Object."));        
        return matte_store_new_value(store);
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);

    if (m->table.attribSet && matte_value_type(*m->table.attribSet)) {
        matteValue_t set = matte_value_object_access(store, *m->table.attribSet, store->specialString_keys, 0);

        if (matte_value_type(set)) {
            v = matte_vm_call(store->vm, set, matte_array_empty(), matte_array_empty(), NULL);
            return matte_value_object_values(store, v);
        }    
    }
    matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;


    // then string
    matteArray_t * keyed = store->kvIter_k;
    keyed->size = 0;
    if (m->table.keyvalues_id) {
        matte_mvt2_get_all_keys(m->table.keyvalues_id, keyed);
        uint32_t len = matte_array_get_size(keyed);
        uint32_t i;
        matteValue_t * iter = (matteValue_t*)matte_array_get_data(keyed);
        for(i = 0; i < len; ++i, iter++) {
            matteValue_t key = (*iter);
            if (matte_value_type(key) == MATTE_VALUE_TYPE_STRING)
                matte_string_store_ref_id(store->stringStore, key.value.id);

            #ifdef MATTE_DEBUG__STORE
                if (matte_value_type(key) == MATTE_VALUE_TYPE_OBJECT)
                    matte_store_track_in(store, key, "object.keys()", 0);
            #endif


            matte_array_push(keys, key);                
        }
    }
    uint32_t i;
    uint32_t len;
    if (!QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE) &&
        !QUERY_STATE(m, OBJECT_STATE__HAS_LAYOUT)) {
    

        // first number 
        len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
        if (len) {
            for(i = 0; i < len; ++i) {
                matteValue_t key = matte_store_new_value(store);
                matte_value_into_number(store, &key, i);
                matte_array_push(keys, key);
            }
        }

    }
    

    
    val = matte_store_new_value(store);
    matte_value_into_new_object_array_ref(store, &val, keys);
    len = matte_array_get_size(keys);
    for(i = 0; i < len; ++i) {
        matte_store_recycle(store, matte_array_at(keys, matteValue_t, i));
    }
    matte_array_destroy(keys);
    return val;
}

matteValue_t matte_value_object_values(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "Can only get keys from something that's an Object."));        
        return matte_store_new_value(store);
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (m->table.attribSet && matte_value_type(*m->table.attribSet)) {
        matteValue_t set = matte_value_object_access(store, *m->table.attribSet, store->specialString_values, 0);

        if (matte_value_type(set)) {
            v = matte_vm_call(store->vm, set, matte_array_empty(), matte_array_empty(), NULL);
            m = matte_store_bin_fetch_table(store->bin, v.value.id);

            if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
                matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "values attribute MUST return an object."));
                return matte_store_new_value(store);
            } else {
                m = matte_store_bin_fetch_table(store->bin, v.value.id);
            }
        }    
    }


    matteArray_t * vals = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 

    matteArray_t * keyed = store->kvIter_v;
    keyed->size = 0;
    // then string
    if (m->table.keyvalues_id) {
        matte_mvt2_get_all_values(m->table.keyvalues_id, keyed);
        if (keyed->size) {
            uint32_t len = matte_array_get_size(keyed);
            uint32_t i;
            matteValue_t * iter = (matteValue_t*)matte_array_get_data(keyed);

            if (QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE)) {
                for(i = 0; i < len; ++i, iter++) {
                    matteValue_t * val = iter;

                    if (IS_FUNCTION_ID(val->value.id)) {
                        matteValue_t vv = matte_store_new_value(store);
                        matte_value_into_copy(store, &vv, *val);                    
                        matte_array_push(vals, vv);
                        continue;                
                    }
                    matteObject_t * setget = matte_store_bin_fetch_table(store->bin, val->value.id);

                    if (setget->table.keyvalues_id == NULL) {
                        continue;
                    }
                    matteValue_t * getter = matte_mvt2_find(setget->table.keyvalues_id, store->specialString_get);
                    if (getter == NULL) {
                        continue;
                    }

                    matteValue_t vv = matte_vm_call_full(
                        store->vm, 
                        *getter, 
                        matte_value_object_get_interface_private_binding_unsafe(store, v),
                        matte_array_empty(), 
                        matte_array_empty(), 
                        NULL
                    );

                    m = matte_store_bin_fetch_table(store->bin, v.value.id);
                    matte_array_push(vals, vv);                
                }    
            } else {
                for(i = 0; i < len; ++i, iter++) {
                    val = matte_store_new_value(store);
                    matte_value_into_copy(store, &val, *iter);
                    matte_array_push(vals, val);                
                }    
            
            }
            keyed->size = 0;
        }        
    }    
    
    
    uint32_t i;
    uint32_t len;

    if (!QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE) &&
        !QUERY_STATE(m, OBJECT_STATE__HAS_LAYOUT)) {
        len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;

        if (len) {
            for(i = 0; i < len; ++i) {
                val = matte_store_new_value(store);
                matte_value_into_copy(store, &val, matte_array_at(m->table.keyvalues_number, matteValue_t, i));
                matte_array_push(vals, val);
            }
        }
    }        


    val = matte_store_new_value(store);
    matte_value_into_new_object_array_ref(store, &val, vals);
    len = matte_array_get_size(vals);
    for(i = 0; i < len; ++i) {
        matte_store_recycle(store, matte_array_at(vals, matteValue_t, i));
    }
    matte_array_destroy(vals);
    return val;
}

// Returns the number of number keys within the object, ignoring keys of other types.
uint32_t matte_value_object_get_number_key_count(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "Can only get size from something that's an Object."));        
        return 0;
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    return m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
}


uint32_t matte_value_object_get_key_count(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "Can only get a keycount from something that's an Object."));        
        return 0;
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    uint32_t total = 
        (m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0) +
        (m->table.keyvalues_id ? matte_mvt2_get_size(m->table.keyvalues_id) : 0);

    return total;    
}

void matte_value_object_remove_key_string(matteStore_t * store, matteValue_t v, const matteString_t * kstr) {
    matteValue_t k = matte_store_new_value(store);
    matte_value_into_string(store, &k, kstr);
    matte_value_object_remove_key(store, v, k);
    matte_store_recycle(store, k);
}

void matte_value_object_remove_key(matteStore_t * store, matteValue_t v, matteValue_t key) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "Can only remove from something that's an Object."));        
        return;
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    switch(matte_value_type(key)) {
      case MATTE_VALUE_TYPE_EMPTY: return;

      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        uint32_t index = (uint32_t) matte_value_get_number(key);
        if (!m->table.keyvalues_number) return;
        if (index >= matte_array_get_size(m->table.keyvalues_number)) {
            return;
        }
        matteValue_t * value = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
        if (matte_value_type(*value) == MATTE_VALUE_TYPE_OBJECT) {
            object_unlink_parent_value(store, m, value);                
        }

        matte_store_recycle(store, *value);
        matte_array_remove(m->table.keyvalues_number, index);
        return;
      }


      case MATTE_VALUE_TYPE_STRING: {
        if (!m->table.keyvalues_id) return;
        matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);
        if (value) {
            if (matte_value_type(*value) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, m, value);                
            }
            matte_mvt2_remove(m->table.keyvalues_id, key);
            matte_store_recycle(store, *value);
            matte_string_store_unref(store->stringStore, key.value.id);
        }
        return;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: 
      case MATTE_VALUE_TYPE_BOOLEAN: 
      case MATTE_VALUE_TYPE_TYPE: {
        if (!m->table.keyvalues_id) return;
        matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);
        if (value) {
            if (matte_value_type(*value) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, m, value);                
            }
            
            if (matte_value_type(key) == MATTE_VALUE_TYPE_OBJECT) {
                object_unlink_parent_value(store, m, &key);            
            }
            matte_mvt2_remove(m->table.keyvalues_id, key);
            matte_store_recycle(store, *value);
        }
        return;
      }
    }

}

void matte_value_object_foreach(matteStore_t * store, matteValue_t v, matteValue_t func) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT || IS_FUNCTION_ID(v.value.id)) {
        matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "'foreach' requires an object."));
        return;
    }


    // 
    matteObject_t * m = (matteObject_t*)matte_store_bin_fetch_table(store->bin, v.value.id);

    // foreach operator
    if (m->table.attribSet && matte_value_type(*m->table.attribSet)) {
        matteValue_t set = matte_value_object_access(store, *m->table.attribSet, store->specialString_foreach, 0);

        if (matte_value_type(set)) {
            v = matte_vm_call(store->vm, set, matte_array_empty(), matte_array_empty(), NULL);
            m = (matteObject_t*)matte_store_bin_fetch_table(store->bin, v.value.id);
            if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
                matte_vm_raise_error_string(store->vm, MATTE_VM_STR_CAST(store->vm, "foreach attribute MUST return an object."));
                return;
            } else {
                m = matte_store_bin_fetch_table(store->bin, v.value.id);
            }
        }
    }
    matte_value_object_push_lock(store, v);


    matteValue_t args[] = {
        matte_store_new_value(store),
        matte_store_new_value(store)
    };
    matteValue_t argNames[2] = {
        store->specialString_key,
        store->specialString_value,
    };

    matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(store, func);
    // dynamic name binding for foreach
    if (stub && matte_bytecode_stub_arg_count(stub) >= 2) {
        argNames[0] = matte_bytecode_stub_get_arg_name_noref(stub, 0);
        argNames[1] = matte_bytecode_stub_get_arg_name_noref(stub, 1);
    }

    matteArray_t argNames_array = MATTE_ARRAY_CAST(argNames, matteValue_t, 2);

    matteArray_t *keys   = matte_array_create(sizeof(matteValue_t));
    matteArray_t *values = matte_array_create(sizeof(matteValue_t));

    // first number 
    uint32_t i;
    uint32_t len;

    matteArray_t * keyIter = matte_array_create(sizeof(matteValue_t));
    matteArray_t * valIter = matte_array_create(sizeof(matteValue_t));
    
    keyIter->size = 0;
    valIter->size = 0;

    // then string
    if (m->table.keyvalues_id && matte_mvt2_get_size(m->table.keyvalues_id)) {
        matte_mvt2_get_all_keys(m->table.keyvalues_id, keyIter);
        matte_mvt2_get_all_values(m->table.keyvalues_id, valIter);
        len = matte_array_get_size(keyIter);


        if (QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE)) {
            for(i = 0; i < len; ++i) {
                args[1] = matte_array_at(valIter, matteValue_t, i);
                args[0] = matte_array_at(keyIter, matteValue_t, i);
                
                if (!IS_FUNCTION_ID(args[1].value.id)) {
                
                    matteObject_t * setget = matte_store_bin_fetch_table(store->bin, args[1].value.id);
                    if (setget->table.keyvalues_id == NULL) {
                        continue;
                    }
                    matteValue_t * getter = matte_mvt2_find(setget->table.keyvalues_id, store->specialString_get);
                    if (getter == NULL) {
                        continue;
                    }

                    
                    args[1] = matte_vm_call_full(
                        store->vm, 
                        *getter, 
                        matte_value_object_get_interface_private_binding_unsafe(store, v),
                        matte_array_empty(), 
                        matte_array_empty(), 
                        NULL
                    );
                    m = (matteObject_t*)matte_store_bin_fetch_table(store->bin, v.value.id);
                }
                
                matte_value_object_push_lock(store, args[1]);
                matte_array_push(keys, args[0]);
                matte_array_push(values, args[1]);
            }
        
        } else {
            for(i = 0; i < len; ++i) {
                args[1] = matte_array_at(valIter, matteValue_t, i);
                args[0] = matte_array_at(keyIter, matteValue_t, i);

                matte_value_object_push_lock(store, args[0]);                
                matte_value_object_push_lock(store, args[1]);
                
                matte_array_push(keys, args[0]);
                matte_array_push(values, args[1]);
            }
        }
        args[0].binIDreserved = 0;
        
    }
    if (!QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE) &&
        !QUERY_STATE(m, OBJECT_STATE__HAS_LAYOUT)) {

        len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
        if (len) {
            for(i = 0; i < len; ++i) {
                args[1] = matte_array_at(m->table.keyvalues_number, matteValue_t, i);
                matte_value_into_number(store, &args[0], i);
                matte_value_object_push_lock(store, args[1]);
                matte_array_push(keys, args[0]);
                matte_array_push(values, args[1]);
            }
            args[0].binIDreserved = 0;
        }
    }
    
    // now we can safely iterate
    len = matte_array_get_size(keys);
    for(i = 0; i < len; ++i) {
        args[0] = matte_array_at(keys, matteValue_t, i);
        args[1] = matte_array_at(values, matteValue_t, i);

        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(store->vm, func, &arr, &argNames_array, NULL);
        matte_store_recycle(store, r);
    
    
        matte_value_object_pop_lock(store, args[0]);
        matte_value_object_pop_lock(store, args[1]);
    }
    
    matte_array_destroy(keys);
    matte_array_destroy(values);
    matte_array_destroy(keyIter);
    matte_array_destroy(valIter);
    
    matte_value_object_pop_lock(store, v);

}

matteValue_t matte_value_subset(matteStore_t * store, matteValue_t v, uint32_t from, uint32_t to) {
    matteValue_t out = matte_store_new_value(store);

    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        uint32_t curlen = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;

        if (from > to || from >= curlen || to >= curlen) {
          matte_value_into_new_object_ref(store, &out);
          return out;
        }


        matteArray_t arr = MATTE_ARRAY_CAST(
            ((matteValue_t *)matte_array_get_data(m->table.keyvalues_number))+from,
            matteValue_t,
            (to-from)+1
        );
        matte_value_into_new_object_array_ref(
            store,
            &out,
            &arr
        );

        break;
      }


      case MATTE_VALUE_TYPE_STRING: {
        const matteString_t * str = matte_string_store_find(store->stringStore, v.value.id);
        uint32_t curlen = matte_string_get_length(str);
        if (from >= curlen || to >= curlen) {
          matteString_t * mstr = matte_string_create_from_c_str("");
          matte_value_into_string(store, &out, mstr);
          matte_string_destroy(mstr);
          return out;
        }

        matte_value_into_string(store, &out, matte_string_get_substr(str, from, to));
      }


    }
    return out;
}



void matte_value_object_set_attributes(matteStore_t * store, matteValue_t v, matteValue_t opObject) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Cannot assign attributes set to something that isnt an object.");
        return;
    }
    
    if (matte_value_type(opObject) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Cannot assign attributes set that isn't an object.");
        return;
    }
    
    if (opObject.value.id == v.value.id) {
        matte_vm_raise_error_cstring(store->vm, "Cannot assign attributes set as its own object.");
        return;    
    }
    
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (m->table.attribSet && matte_value_type(*m->table.attribSet)) {
        object_unlink_parent_value(store, m, m->table.attribSet);
        matte_store_recycle(store, *m->table.attribSet);
    }
    if (!m->table.attribSet)
        m->table.attribSet = (matteValue_t*)matte_allocate(sizeof(matteValue_t));
    
    matte_value_into_copy(store, m->table.attribSet, opObject);
    object_link_parent_value(store, m, m->table.attribSet);
}


int matte_value_object_get_is_interface_unsafe(
    matteStore_t * store, matteValue_t v
) {
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    return QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE) != 0;
}

matteValue_t matte_value_object_get_interface_private_binding_unsafe(
    matteStore_t * store, matteValue_t v
) {
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (m->table.privateBinding == NULL)
        return matte_store_new_value(store);
    return *m->table.privateBinding;
}



void matte_value_object_set_is_interface(
    matteStore_t * store, matteValue_t v, int enable, matteValue_t dyn
) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Cannot enable/disable interface mode for something that isn't an Object.");
        return;
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (enable)
        ENABLE_STATE(m, OBJECT_STATE__HAS_INTERFACE);    
    else
        DISABLE_STATE(m, OBJECT_STATE__HAS_INTERFACE);    

    if (m->table.privateBinding) {
        object_unlink_parent_value(store, m, m->table.privateBinding);
        matte_store_recycle(store, *m->table.privateBinding);       
        matte_deallocate(m->table.privateBinding);     
        m->table.privateBinding = NULL;
    }
    
    if (matte_value_type(dyn)) {
        if (matte_value_type(dyn) != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(store->vm, "Cannot use a custom dynamic binding for an interface that isn't an Object.");
            return;
        }
        m->table.privateBinding = (matteValue_t*)matte_allocate(sizeof(matteValue_t));
        matte_value_into_copy(store, m->table.privateBinding, dyn);
        object_link_parent_value(store, m, m->table.privateBinding);
    }
    
}





const matteValue_t * matte_value_object_get_attributes_unsafe(matteStore_t * store, matteValue_t v) {
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    if (m->table.attribSet && matte_value_type(*m->table.attribSet)) return m->table.attribSet;
    return NULL;
}



typedef struct {
    matteValue_t fn;
    matteArray_t names;
    matteStore_t * store;
} matteParams_Sort_t;

typedef struct {
    matteValue_t value;
    matteParams_Sort_t * sort;
} matteValue_Sort_t;  

static int matte_value_object_sort__cmp(
    const void * asrc,
    const void * bsrc
) {
    matteValue_Sort_t * a = (matteValue_Sort_t *)asrc;
    matteValue_Sort_t * b = (matteValue_Sort_t *)bsrc;

    matteValue_t args[] = {
        a->value,
        b->value
    };
    matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
    return matte_value_as_number(a->sort->store, matte_vm_call(a->sort->store->vm, a->sort->fn, &arr, &a->sort->names, NULL));    
}


void matte_value_object_sort_unsafe(matteStore_t * store, matteValue_t v, matteValue_t less) {


    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);

    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
    if (len < 1) return;

    matteParams_Sort_t params;
    params.fn = less;
    params.store = store;
    
    matte_value_object_push_lock(store, v);
    matte_value_object_push_lock(store, less);

    matteValue_t names[2];
    names[0].binIDreserved = MATTE_VALUE_TYPE_STRING;
    names[1].binIDreserved = MATTE_VALUE_TYPE_STRING;
    names[0].value.id = matte_string_store_ref_cstring(store->stringStore, "a");
    names[1].value.id = matte_string_store_ref_cstring(store->stringStore, "b");
    params.names = MATTE_ARRAY_CAST(names, matteValue_t, 2);
    
    matteValue_Sort_t * sortArray = (matteValue_Sort_t*)matte_allocate(sizeof(matteValue_Sort_t)*len);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        sortArray[i].value = matte_array_at(m->table.keyvalues_number, matteValue_t, i);
        sortArray[i].sort = &params;
    }
    
    qsort(sortArray, len, sizeof(matteValue_Sort_t), matte_value_object_sort__cmp);
    
    for(i = 0; i < len; ++i) {
        matte_array_at(m->table.keyvalues_number, matteValue_t, i) = sortArray[i].value;
    }
    matte_deallocate(sortArray);
    
    matte_value_object_pop_lock(store, v);
    matte_value_object_pop_lock(store, less);

   
}



void matte_value_object_insert(
    matteStore_t * store, 
    matteValue_t v, 
    uint32_t index, 
    matteValue_t val
) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        return;
    }

    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);

    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {    
        object_link_parent_value(store, m, &val);
    }
    if (!m->table.keyvalues_number) m->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));//(matteArray_t*)matte_pool_fetch(store->keyvalues_numberPool);
    matte_array_insert(
        m->table.keyvalues_number,
        index,
        val
    );
}


void matte_value_object_push(
    matteStore_t * store, 
    matteValue_t v, 
    matteValue_t val
) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        return;
    }

    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);

    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {    
        object_link_parent_value(store, m, &val);
    }
    
    matteValue_t vv = matte_store_new_value(store);
    matte_value_into_copy(store, &vv, val);
    if (!m->table.keyvalues_number) m->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));//(matteArray_t*)matte_pool_fetch(store->keyvalues_numberPool);
    matte_array_push(
        m->table.keyvalues_number,
        vv
    );
}

void matte_value_object_set_table(matteStore_t * store, matteValue_t v, matteValue_t srcTable) {
    if (matte_value_type(srcTable) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Cannot use object set assignment syntax something that isnt an object.");
        return;
    }

    matteValue_t keys = matte_value_object_keys(store, srcTable);
    matte_value_object_push_lock(store, keys);
    
    uint32_t len = matte_value_object_get_key_count(store, keys);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        matteValue_t * key = matte_value_object_array_at_unsafe(store, keys, i);
        matteValue_t val = matte_value_object_access(store, srcTable, *key, 1); // bracket source
        
        matte_value_object_set(store, v, *key, val, 0); // dot target
    } 

    matte_value_object_pop_lock(store, keys);
}



matteValue_t matte_value_object_set_key_string(matteStore_t * store, matteValue_t obj, const matteString_t * key, matteValue_t value) {
    matteValue_t keyv = matte_store_new_value(store);
    matte_value_into_string(store, &keyv, key);
    matteValue_t out = matte_value_object_set(store, obj, keyv, value, 0);
    matte_store_recycle(store, keyv);
    return out;
}



matteValue_t matte_value_object_set(matteStore_t * store, matteValue_t v, matteValue_t key, matteValue_t value, int isBracket) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(store->vm, "Cannot set property on something that isnt an object.");
        return matte_store_new_value(store);
    }
    if (matte_value_type(key) == 0) {
        matte_vm_raise_error_cstring(store->vm, "Cannot set property with an empty key");
        return matte_store_new_value(store);
    }
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);

    int hasInterface = QUERY_STATE(m, OBJECT_STATE__HAS_INTERFACE);
    
    matteValue_t assigner = {};
    if (hasInterface == 0 || (hasInterface && isBracket))
        assigner = object_get_access_operator(store, m, isBracket, 0);
    
    if (hasInterface && matte_value_type(assigner) == 0) {
        matteValue_t val = value;

        if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) {
            matte_vm_raise_error_cstring(store->vm, "Objects with interfaces only have string-keyed members.");
            return matte_store_new_value(store);
        }

        if (!m->table.keyvalues_id) {
            matte_vm_raise_error_cstring(store->vm, "Object's interface was empty.");
            return matte_store_new_value(store);
        }        
        
        matteValue_t * value = matte_mvt2_find(m->table.keyvalues_id, key);
        if (value == NULL) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object's interface has no member \"%s\".",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return matte_store_new_value(store);
        }
        
        if (matte_value_type(*value) != MATTE_VALUE_TYPE_OBJECT) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object's interface member \"%s\" is neither a Function nor an Object. Interface is malformed.",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return matte_store_new_value(store);
        
        }
        
        if (IS_FUNCTION_ID(value->value.id)) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object's \"%s\" is a member function and is read-only. Writing to this member is not allowed.",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return matte_store_new_value(store);
        }
        
        matteObject_t * setget = matte_store_bin_fetch_table(store->bin, value->value.id);
        if (setget->table.keyvalues_id == NULL) {
            return matte_store_new_value(store);
        }
        matteValue_t * setter = matte_mvt2_find(setget->table.keyvalues_id, store->specialString_set);
        if (!setter) {
            matteString_t * err = matte_string_create_from_c_str(
                "Object's interface disallows writing of member \"%s\".",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, key))
            );
            matte_vm_raise_error_string(store->vm, err);
            matte_string_destroy(err);
            return matte_store_new_value(store);
        }

        

        matteValue_t args[] = {
            val
        };

        matteValue_t argNames[] = {
            store->specialString_value,
        };
        matteArray_t argNames_array = MATTE_ARRAY_CAST(argNames, matteValue_t, 1);

        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 1);
        matteValue_t privateBinding = {};
        if (m->table.privateBinding) {
            privateBinding = *m->table.privateBinding;
        }



        matte_vm_call_full(store->vm, *setter, privateBinding, &arr, &argNames_array, NULL);
        return matte_store_new_value(store);
    }

    if (matte_value_type(assigner)) {
        matteValue_t args[2] = {
            key, value
        };
        matteValue_t argNames[] = {
            store->specialString_key,
            store->specialString_value,
        };
        matteArray_t argNames_array = MATTE_ARRAY_CAST(argNames, matteValue_t, 2);

        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(store->vm, assigner, &arr, &argNames_array, NULL);
        matte_store_recycle(store, assigner);
        return r;
    } else {
        matteValue_t * v = object_put_prop(store, m, key, value);
        if (!v) return matte_store_new_value(store);
        matteValue_t out = matte_store_new_value(store);
        matte_value_into_copy(store, &out, *v);
        return out;
    }
}

void matte_value_object_set_index_unsafe(matteStore_t * store, matteValue_t v, uint32_t index, matteValue_t val) {
    matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
    matteValue_t * newV = &matte_array_at(m->table.keyvalues_number, matteValue_t, index);
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_copy(store, &out, val);

    // track reference
    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {
        object_link_parent_value(store, m, &val);
    }
    if (matte_value_type(*newV) == MATTE_VALUE_TYPE_OBJECT) {
        object_unlink_parent_value(store, m, newV);
    }

    matte_store_recycle(store, *newV);
    *newV = out;
}


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy_(matteStore_t * store, matteValue_t * to, matteValue_t from) {

        

    switch(matte_value_type(from)) {
      case MATTE_VALUE_TYPE_EMPTY:
      case MATTE_VALUE_TYPE_NUMBER: 
      case MATTE_VALUE_TYPE_TYPE: 
      case MATTE_VALUE_TYPE_BOOLEAN: 
        *to = from;
        break;
      case MATTE_VALUE_TYPE_STRING: 
        *to = from;
        matte_string_store_ref_id(store->stringStore, from.value.id);
        return;

      case MATTE_VALUE_TYPE_OBJECT: {
        if (to->binIDreserved == from.binIDreserved && 
            to->value.id == from.value.id
            ) return;

        matte_store_recycle(store, *to);
        *to = from;
        return;
      }
    }
}

uint32_t matte_value_type_get_typecode(matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_TYPE) return 0;
    return v.value.id;
}


int matte_value_isa(matteStore_t * store, matteValue_t v, matteValue_t typeobj) {
    if (matte_value_type(typeobj) != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_cstring(store->vm, "VM error: cannot query isa() with a non Type value.");
        return 0;
    }
    if (typeobj.value.id == store->type_any.value.id) return 1;
    if (matte_value_type(v) != MATTE_VALUE_TYPE_OBJECT) {
        uint32_t typ = matte_value_get_type(store, v).value.id;
        if (typeobj.value.id == store->type_nullable.value.id && matte_value_type(v) == MATTE_VALUE_TYPE_EMPTY) return 1;  
        return typ == typeobj.value.id;
    } else {
        if (typeobj.value.id == store->type_nullable.value.id) return 1;  
        if (IS_FUNCTION_ID(v.value.id))
            return store->type_function.value.id == typeobj.value.id;
            
        if (typeobj.value.id == store->type_object.value.id) return 1;

        matteValue_t typep = matte_value_get_type(store, v);
        if (typep.value.id == typeobj.value.id) return 1;

        MatteTypeData * d = &matte_array_at(store->typecode2data, MatteTypeData, typep.value.id);
        if (!d->isa) return 0;

        uint32_t i;
        uint32_t count = matte_array_get_size(d->isa);
        for(i = 0; i < count; ++i) {
            if (matte_array_at(d->isa, uint32_t, i) == typeobj.value.id) return 1;
        }
        return 0;
    }
}

matteValue_t matte_value_type_name_noref(matteStore_t * store, matteValue_t v) {
    if (matte_value_type(v) != MATTE_VALUE_TYPE_TYPE) {
        matte_vm_raise_error_cstring(store->vm, "VM error: cannot get type name of non Type value.");
        return store->specialString_empty;
    }
    switch(v.value.id) {
      case 1:     return store->specialString_type_empty;
      case 2:     return store->specialString_type_boolean;
      case 3:     return store->specialString_type_number;
      case 4:     return store->specialString_type_string;
      case 5:     return store->specialString_type_object;
      case 6:     return store->specialString_type_function;
      case 7:     return store->specialString_type_type;
      case 8:     return store->specialString_type_any;
      case 9:     return store->specialString_type_nullable;
      default: { 
        if (v.value.id >= matte_array_get_size(store->typecode2data)) {
            matte_vm_raise_error_cstring(store->vm, "VM error: no such type exists...");                    
        } else
            return matte_array_at(store->typecode2data, MatteTypeData, v.value.id).name;
      }
    }
    return store->specialString_nothing;
}



const matteValue_t * matte_store_get_empty_type(matteStore_t * h) {
    return &h->type_empty;
}
const matteValue_t * matte_store_get_boolean_type(matteStore_t * h) {
    return &h->type_boolean;
}
const matteValue_t * matte_store_get_number_type(matteStore_t * h) {
    return &h->type_number;
}
const matteValue_t * matte_store_get_string_type(matteStore_t * h) {
    return &h->type_string;
}
const matteValue_t * matte_store_get_object_type(matteStore_t * h) {
    return &h->type_object;
}
const matteValue_t * matte_store_get_function_type(matteStore_t * h) {
    return &h->type_function;
}
const matteValue_t * matte_store_get_type_type(matteStore_t * h) {
    return &h->type_type;
}
const matteValue_t * matte_store_get_any_type(matteStore_t * h) {
    return &h->type_any;
}
const matteValue_t * matte_store_get_nullable_type(matteStore_t * h) {
    return &h->type_nullable;
}

matteValue_t matte_store_get_dynamic_bind_token_noref(matteStore_t * h) {
    return h->specialString_dynamicBindToken;
}




matteValue_t matte_value_get_type(matteStore_t * store, matteValue_t v) {
    switch(matte_value_type(v)) {
      case MATTE_VALUE_TYPE_EMPTY:   return store->type_empty;
      case MATTE_VALUE_TYPE_BOOLEAN: return store->type_boolean;
      case MATTE_VALUE_TYPE_NUMBER: return store->type_number;
      case MATTE_VALUE_TYPE_STRING: return store->type_string;
      case MATTE_VALUE_TYPE_TYPE: return store->type_type;
      case MATTE_VALUE_TYPE_OBJECT: {
        if (IS_FUNCTION_ID(v.value.id))
            return store->type_function;
        matteObject_t * m = matte_store_bin_fetch_table(store->bin, v.value.id);
        matteValue_t out;
        out.binIDreserved = MATTE_VALUE_TYPE_TYPE;
        out.value.id = m->typecode;
        return out;
      }

    }
    // error out?
    return store->type_empty;
}



// returns a value to the store.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_store_recycle_(
    matteStore_t * store,
    matteValue_t v
#ifdef MATTE_DEBUG__STORE
    ,const char * file_,
    int line_
#endif    
) {
    if (matte_value_type(v) == MATTE_VALUE_TYPE_STRING) {
        matte_string_store_unref(store->stringStore, v.value.id);
    }
    /*if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);
        if (!m) return;
        if (!QUERY_STATE(m, OBJECT_STATE__IS_ETERNAL)) {
            if (!QUERY_STATE(m, OBJECT_STATE__IS_QUESTIONABLE)) {
                ENABLE_STATE(m, OBJECT_STATE__IS_QUESTIONABLE);       
                matte_array_push(store->questionableList, m);
            }
        }
    }*/
    
    
}



static void print_state_bits(matteObject_t * m) {
    printf(
        "OBJECT_STATE__RECYCLED:    %d\n",

        QUERY_STATE(m, OBJECT_STATE__RECYCLED)
    );        
}

////
////
////
//    CYCLE CHECK

/*
    - list of all ROOTS 
    - list of all SUSPECTED cycles (those orphaned by roots recently)
    
    
    1. traverse all roots and tag each reachable with the "REACHES_ROOT" tag 
       also updating the generation to make the iterations generation to 
       make the reaches root tag reliable
       
    2. (fn search) for each suspected cycle value:
        - if it REACHES_ROOT this generation, skip 
        - if it has been visited this cycle, cycle confirmed
        - set visited to true and match generation 
        - foreach child value 
            - search(child)
        
    




*/








void matte_store_push_lock_gc(matteStore_t * h) {
    h->gcLocked++;
}
void matte_store_pop_lock_gc(matteStore_t * h) {
    h->gcLocked--;
}


/* standard */



// distributed
/*
#define GC_PARAM__CALL_MOD 60
#define GC_PARAM__MARCH_SIZE 180
#define GC_PARAM__ROOT_CHUNK 25
*/




#ifdef MATTE_DEBUG__STORE
/*
static void assert_tricolor_invariant(matteStore_t * h, matteObject_t *);

static void assert_tricolor_invariant__child(matteStore_t * h, matteObject_t * parent, matteObject_t * o) {

    if ((o->color      == OBJECT_TRICOLOR__BLACK &&
         parent->color == OBJECT_TRICOLOR__WHITE)
         
        ||


        (o->color      == OBJECT_TRICOLOR__WHITE &&
         parent->color == OBJECT_TRICOLOR__BLACK)) {
        assert(!"Invariant broken.");   
     }
     assert_tricolor_invariant(h, o);

}
*/

#endif








static void destroy_object(void * d) {
    matteObject_t * out = (matteObject_t*)d;
    if (out->storeID == 0) return;
    out->storeID = 0;
    #ifdef MATTE_DEBUG__STORE
        matte_array_destroy(out->parents);
    #endif
    if (out->children) {
        out->children = 0;
    }

    if (IS_FUNCTION_OBJECT(out)) {
        matte_deallocate(out->function.vars);
    } else {

        if (out->table.keyvalues_id) matte_mvt2_destroy(out->table.keyvalues_id);
    }

}
















struct matteStoreBin_t {
    mattePool_t * functions;
    mattePool_t * tables;
};





matteStoreBin_t * matte_store_bin_create() {
    matteStoreBin_t * store = (matteStoreBin_t*)matte_allocate(sizeof(matteStoreBin_t));
    
    store->functions = matte_pool_create(sizeof(matteObject_t), destroy_object);
    store->tables    = matte_pool_create(sizeof(matteObject_t), destroy_object);

    // the 0th object. Nonexistent;
    matte_store_bin_add_function(store);

    // empty function
    matteObject_t * o = matte_store_bin_add_function(store);
    o->function.stub = matte_bytecode_stub_create_symbolic();
 
    
    
    return store;
}

matteObject_t * matte_store_bin_add_function(matteStoreBin_t * store) {
    uint32_t id = matte_pool_add(store->functions);
    matteObject_t * o = matte_pool_fetch(store->functions, matteObject_t, id);
    
    #ifdef MATTE_DEBUG__STORE
        assert(o->storeID == 0 || o->storeID == 0xffffffff);
        if (o->parents == NULL)
            o->parents = matte_array_create(sizeof(matteValue_t));
    #endif
    if (o->function.vars == NULL)
        o->function.vars = (matteVariableData_t*)matte_allocate(sizeof(matteVariableData_t));
    o->storeID = id*2;
	    
    return o;
}

matteObject_t * matte_store_bin_add_table(matteStoreBin_t * store) {
    uint32_t id = matte_pool_add(store->tables);
    matteObject_t * o = matte_pool_fetch(store->tables, matteObject_t, id);
    
    
    #ifdef MATTE_DEBUG__STORE
        assert(o->storeID == 0 || o->storeID == 0xffffffff);
        if (o->parents == NULL)
            o->parents = matte_array_create(sizeof(matteValue_t));
    #endif

    o->storeID = id*2+1;
    return o;

}

matteObject_t * matte_store_bin_fetch(matteStoreBin_t * store, uint32_t id) {
    #ifdef MATTE_DEBUG__STORE
        assert(id != 0);
    #endif

    // function (is even)
    if (IS_FUNCTION_ID(id)) {
        id /= 2;
        return matte_pool_fetch(store->functions, matteObject_t, id);
    // is object
    } else {
        id /= 2;
        return matte_pool_fetch(store->tables, matteObject_t, id);    
    }
}

matteObject_t * matte_store_bin_fetch_function(matteStoreBin_t * store, uint32_t id) {
    #ifdef MATTE_DEBUG__STORE
        assert(id != 0);
    #endif

    #ifdef MATTE_DEBUG__STORE
        assert(IS_FUNCTION_ID(id));
    #endif
    id /= 2;
    return matte_pool_fetch(store->functions, matteObject_t, id);
}

matteObject_t * matte_store_bin_fetch_table(matteStoreBin_t * store, uint32_t id) {
    #ifdef MATTE_DEBUG__STORE
        assert(!IS_FUNCTION_ID(id));
    #endif
    id /= 2;
    return matte_pool_fetch(store->tables, matteObject_t, id);
}

void matte_store_bin_recycle(matteStoreBin_t * store, uint32_t id) {
    // function (is even)
    if (IS_FUNCTION_ID(id)) {
        matte_pool_recycle(store->functions, id/2);
    // is object
    } else {
        matte_pool_recycle(store->tables, id/2);
    }
}

void matte_store_bin_destroy(matteStoreBin_t * store) {
    matte_pool_destroy(store->tables);
    matte_pool_destroy(store->functions);
    matte_deallocate(store);
}






struct matteObjectPool_t {
    matteArray_t * pool;
    void * (*create)();
    void (*destroy)(void*);
};





 

#ifdef MATTE_DEBUG__STORE 


typedef struct {
    matteValue_t refid;
    int refCount;
    matteArray_t * historyFiles;
    matteArray_t * historyLines;
    matteArray_t * historyIncr;
} MatteStoreTrackInfo_Instance;

typedef struct {
    matteTable_t * values[MATTE_VALUE_TYPE_TYPE+1];
    int resolved;
    int unresolved;
} MatteStoreTrackInfo;


static MatteStoreTrackInfo * store_track_info = NULL;


static void matte_store_track_initialize() {
    store_track_info = matte_allocate(sizeof(MatteStoreTrackInfo));
    store_track_info->values[MATTE_VALUE_TYPE_OBJECT] = matte_table_create_hash_pointer();

}

static const char * get_track_store_symbol(int i) {
    switch(i) {
      case 0: return "--";
      case 1: return "++";
      case 2: return "->";
    }
    return "";
}


static void matte_store_track_print(matteStore_t * store, uint32_t id) {
    MatteStoreTrackInfo_Instance * inst = matte_table_find_by_uint(store_track_info->values[MATTE_VALUE_TYPE_OBJECT], id);
    if (!inst) {
        printf("No such object / value.\n");
        return;
    }

    matte_value_print(store, inst->refid);
    uint32_t i;
    uint32_t len = matte_array_get_size(inst->historyFiles);
    for(i = 0; i < len; ++i) {
        printf(
            "%s %s:%d\n", 
            get_track_store_symbol(matte_array_at(inst->historyIncr, int, i)),
            matte_string_get_c_str(matte_array_at(inst->historyFiles, matteString_t *, i)),
            matte_array_at(inst->historyLines, int, i)
        );
    }
}


static void matte_store_report_val(matteStore_t * store, matteTable_t * t) {
    matteTableIter_t * iter = matte_table_iter_create();
    for(matte_table_iter_start(iter, t);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        MatteStoreTrackInfo_Instance * inst = matte_table_iter_get_value(iter);
        //if (!inst->refCount) continue;
        matte_value_print(store, inst->refid);
        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            printf(
                "%s %s:%d\n", 
                get_track_store_symbol(matte_array_at(inst->historyIncr, int, i)),
                matte_string_get_c_str(matte_array_at(inst->historyFiles, matteString_t *, i)),
                matte_array_at(inst->historyLines, int, i)
            );
        }
    }
}

matteValue_t matte_store_track_in_lock(matteStore_t * store, matteValue_t val, const char * file, int line) {
    matteString_t * st = matte_string_create_from_c_str("%s (LOCK PUSHED)", file);
    matteValue_t out = matte_store_track_in(store, val, matte_string_get_c_str(st), line);
    matte_string_destroy(st);
    return out;
}

void matte_store_track_out_lock(matteStore_t * store, matteValue_t val, const char * file, int line) {
    matteString_t * st = matte_string_create_from_c_str("%s (LOCK POPPED)", file);
    matte_store_track_out(store, val, matte_string_get_c_str(st), line);
    matte_string_destroy(st);
}


matteValue_t matte_store_track_in(matteStore_t * store, matteValue_t val, const char * file, int line) {
    if (!store_track_info) {
        matte_store_track_initialize();
    }
    if (!matte_value_type(val) || (matte_value_type(val) != MATTE_VALUE_TYPE_OBJECT)) return val;    
    matteObject_t * o = matte_store_bin_fetch(store->bin, val.value.id);
    MatteStoreTrackInfo_Instance * inst = matte_table_find_by_uint(store_track_info->values[matte_value_type(val)], val.value.id);


    // reference is complete. REMOVE!!!

    if (!inst) {
        inst = matte_allocate(sizeof(MatteStoreTrackInfo_Instance));
        inst->historyFiles = matte_array_create(sizeof(matteString_t *));
        inst->historyLines = matte_array_create(sizeof(int));
        inst->historyIncr  = matte_array_create(sizeof(int));
        matte_table_insert_by_uint(store_track_info->values[matte_value_type(val)], val.value.id, inst);
        store_track_info->unresolved++;
        inst->refid = val;
    } 

    matteString_t * fdup = matte_string_create_from_c_str("%s", file);
    int t = 1;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);
    inst->refCount++;
    return val;
}

void matte_store_track_neutral(matteStore_t * store, matteValue_t val, const char * file, int line) {
    if (!store_track_info) {
        matte_store_track_initialize();
    }
    if (!matte_value_type(val) ||!(matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT)) return;
    matteObject_t * o = matte_store_bin_fetch(store->bin, val.value.id);

    MatteStoreTrackInfo_Instance * inst = matte_table_find_by_uint(store_track_info->values[matte_value_type(val)], val.value.id);


    matteString_t * fdup = matte_string_create_from_c_str("%s", file);
    int t = 2;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);


}


void matte_store_track_out(matteStore_t * store, matteValue_t val, const char * file, int line) {
    if (!store_track_info) {
        matte_store_track_initialize();
    }
    if (!matte_value_type(val) ||!(matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT)) return;
    matteObject_t * o = matte_store_bin_fetch(store->bin, val.value.id);

    MatteStoreTrackInfo_Instance * inst = matte_table_find_by_uint(store_track_info->values[matte_value_type(val)], val.value.id);
        
    if (!inst) {
        return;
    }

    matteString_t * fdup = matte_string_create_from_c_str("%s", file);
    int t = 0;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, line);
    matte_array_push(inst->historyIncr, t);

    /*
    if (inst->refCount == 0) {
        printf("---REFCOUNT NEGATIVE DETECTED---\n");
        if (inst->matte_value_type(refid) == MATTE_VALUE_TYPE_OBJECT || inst->matte_value_type(refid) == MATTE_VALUE_TYPE_FUNCTION)
            matte_value_print(store, inst->refid);
        else 
            printf("Value: {%d, %d}\n", inst->matte_value_type(refid), inst->refid.value.id);


        uint32_t i;
        uint32_t len = matte_array_get_size(inst->historyFiles);
        for(i = 0; i < len; ++i) {
            printf(
                "%s %s:%d\n", 
                get_track_store_symbol(matte_array_at(inst->historyIncr, int, i)),
                matte_array_at(inst->historyFiles, char *, i),
                matte_array_at(inst->historyLines, int, i)
            );
        }
        printf("---REFCOUNT NEGATIVE DETECTED---\n");
        assert(!"Negative refcount means something is getting rid of a value after it has been disposed.");
    }
    */
    if (inst->refCount) inst->refCount--;
    
        

}


void matte_store_track_done(matteStore_t * store, matteValue_t val) {
    if (!store_track_info) {
        matte_store_track_initialize();
    }
    if (!matte_value_type(val) ||!(matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT)) return;
    matteObject_t * o = matte_store_bin_fetch(store->bin, val.value.id);

    MatteStoreTrackInfo_Instance * inst = matte_table_find_by_uint(store_track_info->values[matte_value_type(val)], val.value.id);
    if (!inst) {
        assert(!"If this assertion fails, something has used a stray / uninitialized value and tried to destroy it.");
    }

    matteString_t * fdup = matte_string_create_from_c_str("<REMOVED>");
    int t = 0;
    matte_array_push(inst->historyFiles, fdup);
    matte_array_push(inst->historyLines, t);
    matte_array_push(inst->historyIncr, t);

    inst->refCount = 0;

    matte_array_destroy(inst->historyLines);
    matte_array_destroy(inst->historyIncr);
    uint32_t i;
    uint32_t len = matte_array_get_size(inst->historyFiles);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(inst->historyFiles, matteString_t *, i));
    }
    matte_array_destroy(inst->historyFiles);
    matte_table_remove_by_uint(store_track_info->values[matte_value_type(val)], val.value.id);
    matte_deallocate(inst);
    inst = NULL;


}

// prints a report of the store if any references have not 
// 
int matte_store_report(matteStore_t * store) {
    if (!store_track_info) return 0;
    
    if (matte_table_is_empty(store_track_info->values[MATTE_VALUE_TYPE_OBJECT]))   return 0;
    
    

    printf("+---- Matte Store Value Tracker ----+\n");
    printf("|     UNFREED VALUES DETECTED      |\n");
    printf("Object values:\n"); matte_store_report_val(store, store_track_info->values[MATTE_VALUE_TYPE_OBJECT]);
    printf("|                                  |\n");
    printf("+----------------------------------+\n\n");



}


static int matte_value_count_children(matteStore_t * store, matteObject_t * m) {
    uint32_t next = m->children;
    uint32_t count = 0;
    while(next) {
        matteObjectNode_t * n = matte_pool_fetch(store->nodes, matteObjectNode_t, next);        
        count ++;
        next = n->next;
    }
    return count;
}

void matte_store_value_object_mark_created(
    matteStore_t * store,
    matteValue_t v,
    matteVMStackFrame_t * frame
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
    if (frame == NULL) {
        m->fileLine = 0;
        m->fileIDsrc = 0xffffffff;
        return;
    }
    uint32_t instCount;
    const matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);
    const matteBytecodeStub_t * stub = frame->stub;
    matteBytecodeStubInstruction_t inst = program[frame->pc-1];

    uint32_t startingLine = matte_bytecode_stub_get_starting_line(stub);    
    m->fileLine = startingLine + inst.info.lineOffset;
    m->fileIDsrc = matte_bytecode_stub_get_file_id(stub);
}

void matte_store_value_object_mark_created_manual(
    matteStore_t * store,
    matteValue_t v,
    uint32_t fileLine,
    uint32_t fileIDsrc
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
    m->fileLine = fileLine;
    m->fileIDsrc = fileIDsrc;
}



void matte_store_value_object_get_reference_graphology__make_node(
    matteVM_t * vm,
    FILE * f,
    uint32_t id,
    uint32_t fileID,
    uint32_t lineNumber,
    int isSource,
    int size,
    int gcCycles,
    int gcCyclesTotal,
    int rootState
) {
    int isFunction = id/2 == (id+1)/2;

    const matteString_t * str = matte_vm_get_script_name_by_id(vm, fileID);
    fprintf(
        f,
        "{\"key\":\"%d\", \"attributes\":{\"label\":\"%s %d - %s, %d.\\nAlive %d cycles.%s\", \"size\":%f",
        (int) id,
        (isFunction) ? "Fn" : "Obj",
        (int) id,
        (str == NULL ? "???" : matte_string_get_c_str(str)),
        (int)lineNumber,
        gcCycles,
        (rootState) ? "Is a root." : "",
        (float) isSource ? 7 : 3 + (size/(float)10)
    );
    
    if (isSource) {
        fprintf(f, ", \"color\":\"red\"}},\n");
    } else {
        int amt = 0x15 + (int)((0xff - 0x15)*(1 - gcCycles / (float)gcCyclesTotal));
        fprintf(f, ", \"color\":\"#%2x%2x%2x\"}},\n", amt, amt, amt);
    }
}

void matte_store_value_object_get_reference_graphology__make_edge(
    FILE * f,
    uint32_t id0,
    uint32_t id1
) {
    fprintf(
        f,
        "{\"source\":\"%d\", \"target\":\"%d\"},\n", 
        (int) id0,
        (int) id1
    );
}

static void matte_store_value_object_get_reference_graphology_value__scan_object(
    matteStore_t * store,
    matteTable_t * visited,
    matteVM_t * vm,
    matteValue_t v,
    int source,
    FILE * fnodes,
    FILE * fedges,
    int depthLevel
) {
    if (depthLevel <= 0) return;
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
    if (source != 1 && matte_table_find_by_uint(visited, m->storeID)) return;
    
    matte_table_insert_by_uint(visited, m->storeID, (void*)0x1);
    matte_store_value_object_get_reference_graphology__make_node(
        vm,
        fnodes,
        m->storeID,
        m->fileIDsrc,
        m->fileLine,
        source,
        1+matte_value_count_children(store, m),
        (int)(store->gcCycles - m->gcCycles),
        (int) store->gcCycles,
        m->rootState
    );
    
    
    uint32_t i;
    uint32_t len = matte_array_get_size(m->parents);
    for(i = 0; i < len; ++i) {
        matteValue_t v = matte_array_at(m->parents, matteValue_t, i);
        
        matte_store_value_object_get_reference_graphology__make_edge(
            fedges,
            m->storeID,
            v.value.id
        );
        
        matte_store_value_object_get_reference_graphology_value__scan_object(
            store,
            visited,
            vm,
            v,
            0,
            fnodes,
            fedges,
            depthLevel-1
        );
    }
}




void matte_store_value_object_get_reference_graphology_value(
    matteStore_t * store,
    matteVM_t * vm,
    matteValue_t v
) {

    matteTable_t * t = matte_table_create_hash_pointer();
    

    FILE * fnodes = fopen("nodes.js", "w");
    FILE * fedges = fopen("edges.js", "w");

    fprintf(fnodes, "%s", "MATTE_NODES = [\n");
    fprintf(fedges, "%s", "MATTE_EDGES = [\n");

    matte_store_value_object_get_reference_graphology_value__scan_object(
        store,
        t,
        vm,
        v,
        1,
        fnodes,
        fedges,
        1000000
    );

    fprintf(fnodes, "%s", "];\n");
    fprintf(fedges, "%s", "];\n");

    
    fclose(fnodes);   
    fclose(fedges);   
    
    matte_table_destroy(t);
}






void matte_store_value_object_get_reference_graphology_all__scan_object_down(
    matteStore_t * store,
    matteTable_t * visited,
    matteVM_t * vm,
    matteValue_t v,
    int source,
    FILE * fnodes,
    FILE * fedges
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
    if (matte_table_find_by_uint(visited, m->storeID)) return;
    
    matte_table_insert_by_uint(visited, m->storeID, (void*)0x1);
    matte_store_value_object_get_reference_graphology__make_node(
        vm,
        fnodes,
        m->storeID,
        m->fileIDsrc,
        m->fileLine,
        source,
        1+matte_value_count_children(store, m),
        (int)(store->gcCycles - m->gcCycles),
        (int) store->gcCycles,
        m->rootState
    );
    

    uint32_t iter = m->children;
    while(iter) {
        matteObjectNode_t * n = matte_pool_fetch(store->nodes, matteObjectNode_t, iter);
        
        matteValue_t v = {};
        v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
        v.value.id = n->data;
        
        matte_store_value_object_get_reference_graphology__make_edge(
            fedges,
            m->storeID,
            v.value.id
        );
        
        matte_store_value_object_get_reference_graphology_all__scan_object_down(
            store,
            visited,
            vm,
            v,
            0,
            fnodes,
            fedges
        );
        iter = n->next;
    }
}

void matte_store_value_object_get_reference_graphology_all(
    matteStore_t * store,
    matteVM_t * vm
) {

    matteTable_t * t = matte_table_create_hash_pointer();
    

    FILE * fnodes = fopen("nodes.js", "w");
    FILE * fedges = fopen("edges.js", "w");

    fprintf(fnodes, "%s", "MATTE_NODES = [\n");
    fprintf(fedges, "%s", "MATTE_EDGES = [\n");



    uint32_t root = store->roots;
    while(root) {
        matteObjectNode_t * node = matte_pool_fetch(store->nodes, matteObjectNode_t, root);
        matteValue_t v = {};
        v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
        v.value.id = node->data;
    
        matte_store_value_object_get_reference_graphology_all__scan_object_down(
            store,
            t,
            vm,
            v,
            1,
            fnodes,
            fedges
        );

        uint32_t next = node->next;        
        root = next;
    }


    fprintf(fnodes, "%s", "];\n");
    fprintf(fedges, "%s", "];\n");

    
    fclose(fnodes);   
    fclose(fedges);   
    
    matte_table_destroy(t);
}





void matte_store_value_object_get_reference_graphology__scan_object_down(
    matteStore_t * store,
    matteTable_t * visited,
    matteArray_t * hits,
    matteVM_t * vm,
    matteValue_t v,
    uint32_t line,
    uint32_t fileIDsrc
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, v.value.id);            
    if (matte_table_find_by_uint(visited, m->storeID)) return;
    
    matte_table_insert_by_uint(visited, m->storeID, (void*)0x1);
    if (m->fileLine == line &&
        m->fileIDsrc == fileIDsrc) {
        matte_array_push(hits, v.value.id);
    }



    uint32_t iter = m->children;
    while(iter) {
        matteObjectNode_t * n = matte_pool_fetch(store->nodes, matteObjectNode_t, iter);
        
        matteValue_t v = {};
        v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
        v.value.id = n->data;
        
        matte_store_value_object_get_reference_graphology__scan_object_down(
            store,
            visited,
            hits,
            vm,
            v,
            line,
            fileIDsrc
        );
        
        iter = n->next;
    }
}



void matte_store_value_object_get_reference_graphology(
    matteStore_t * store,
    matteVM_t * vm,
    const char * filename,
    uint32_t line,
    int depthLevel
) {
    printf("Looking for %s, line %d...\n", filename, (int)line);

    uint32_t fileID = matte_vm_get_file_id_by_name(vm, MATTE_VM_STR_CAST(vm, filename));
    if (fileID == 0xffffffff) {
        printf("Could not find file as a registered script.\n");
        return;
    } 
    printf("Found file as fileID %d\n", (int)fileID);
    matteTable_t * t = matte_table_create_hash_pointer();
    matteArray_t * hits = matte_array_create(sizeof(uint32_t));

    printf("Traversing node graph...\n");

    uint32_t root = store->roots;
    while(root) {
        matteObjectNode_t * node = matte_pool_fetch(store->nodes, matteObjectNode_t, root);
        matteValue_t v = {};
        v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
        v.value.id = node->data;
    
        matte_store_value_object_get_reference_graphology__scan_object_down(
            store,
            t,
            hits,
            vm,
            v,
            line,
            fileID
        );


        uint32_t next = node->next;        
        root = next;
    }
    matte_table_destroy(t);
    if (matte_array_get_size(hits) == 0) {
        printf("...No live object could be found at location.\n");
        return;
    }
        
    printf("Found %d objects alive from location.\n", (int) matte_array_get_size(hits));

    printf("Generating graph...\n");
    matteValue_t v = {};
    v.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
  
    {
        matteTable_t * t = matte_table_create_hash_pointer();
        

        FILE * fnodes = fopen("nodes.js", "w");
        FILE * fedges = fopen("edges.js", "w");

        fprintf(fnodes, "%s", "MATTE_NODES = [\n");
        fprintf(fedges, "%s", "MATTE_EDGES = [\n");


        uint32_t i;
        uint32_t len = matte_array_get_size(hits);
        for(i = 0; i < len; ++i) {
            matte_table_insert_by_uint(
                t,
                matte_array_at(hits, uint32_t, i),
                (void*)0x1
            );
        }
    
        
        for(i = 0; i < len; ++i) {
            v.value.id = matte_array_at(hits, uint32_t, i);
            matte_store_value_object_get_reference_graphology_value__scan_object(
                store,
                t,
                vm,
                v,
                1,
                fnodes,
                fedges,
                depthLevel
            );
        }
        
        

        fprintf(fnodes, "%s", "];\n");
        fprintf(fedges, "%s", "];\n");

        
        fclose(fnodes);   
        fclose(fedges);   
        
        matte_table_destroy(t);    
    }
    matte_array_destroy(hits);
    printf("...Done. Hits marked in red.\n");

}

uint32_t matte_store_value_object_get_memory_breakdown__estimate_usage(matteObject_t * m) {
    uint32_t totalMemory = 0;
    totalMemory += sizeof(matteObject_t);
    
    if (m->storeID%2 == 0) {
        uint32_t len;
        const matteBytecodeStubCapture_t * capturesRaw = matte_bytecode_stub_get_captures(
            m->function.stub,
            &len
        );
    
        totalMemory += sizeof(matteVariableData_t);
        totalMemory += (m->function.isCloned == 0) ? (sizeof(uint32_t)+sizeof(matteValue_t*)) * len : 0;
        totalMemory += (m->function.vars->referrables) ? 
            (matte_bytecode_stub_arg_count(m->function.stub) + 
             matte_bytecode_stub_local_count(m->function.stub)) * sizeof(matteValue_t) : 0;
    } else {
        totalMemory += sizeof(matteVariableData_t);
        if (m->table.keyvalues_number) {
            totalMemory += sizeof(matteArray_t);
            totalMemory += sizeof(matteValue_t) * m->table.keyvalues_number->allocSize;
        }

        if (m->table.keyvalues_id) {
            totalMemory += matte_mvt2_get_size(m->table.keyvalues_id) * (sizeof(matteValue_t)*2);
        }
        
        if (m->table.attribSet) {
            totalMemory += sizeof(matteValue_t);
        }

        if (m->table.privateBinding) {
            totalMemory += sizeof(matteValue_t);
        }

    }
    return totalMemory;
}


void matte_store_value_object_get_memory_breakdown__find_relevant(
    matteStore_t * store,
    matteTable_t * visited,
    matteArray_t * hits,
    matteVM_t * vm,
    uint32_t id,
    uint32_t fileIDsrc,
    uint32_t * totalMemory
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, id);            
    if (matte_table_find_by_uint(visited, id)) return;
    
    matte_table_insert_by_uint(visited, m->storeID, (void*)0x1);
    *totalMemory += matte_store_value_object_get_memory_breakdown__estimate_usage(m);
    if (m->fileIDsrc == fileIDsrc) {
        matte_array_push(hits, m->storeID);
    }

    uint32_t iter = m->children;
    while(iter) {
        matteObjectNode_t * n = matte_pool_fetch(store->nodes, matteObjectNode_t, iter);
        
        matte_store_value_object_get_memory_breakdown__find_relevant(
            store,
            visited,
            hits,
            vm,
            n->data,
            fileIDsrc,
            totalMemory
        );
        
        iter = n->next;
    }
}


void matte_store_value_object_get_memory_breakdown__track_memory(
    matteStore_t * store,
    matteTable_t * visited,
    matteVM_t * vm,
    uint32_t id,
    uint32_t * totalMemory
) {
    matteObject_t * m = matte_store_bin_fetch(store->bin, id);            
    if (matte_table_find_by_uint(visited, id)) return;
    
    matte_table_insert_by_uint(visited, m->storeID, (void*)0x1);
    *totalMemory += matte_store_value_object_get_memory_breakdown__estimate_usage(m);

    uint32_t iter = m->children;
    while(iter) {
        matteObjectNode_t * n = matte_pool_fetch(store->nodes, matteObjectNode_t, iter);
        
        matte_store_value_object_get_memory_breakdown__track_memory(
            store,
            visited,
            vm,
            n->data,
            totalMemory
        );
        
        iter = n->next;
    }
}


typedef struct {
    matteObject_t * obj;
    uint32_t reachableCount;
    uint32_t estimatedWeight;
    double fraction;
} DataLayoutSummary;

int matte_store_value_object_get_memory_breakdown__less(
    const void * a,
    const void * b
) {
    DataLayoutSummary * al = (DataLayoutSummary*)a;
    DataLayoutSummary * bl = (DataLayoutSummary*)b;
    
    return al->reachableCount >= bl->reachableCount;
}


void matte_store_value_object_get_memory_breakdown(
    matteStore_t * store,
    matteVM_t * vm,
    const char * filename,
    int nResults
) {
    printf("Looking for %s...\n", filename);

    uint32_t fileID = matte_vm_get_file_id_by_name(vm, MATTE_VM_STR_CAST(vm, filename));
    if (fileID == 0xffffffff) {
        printf("Could not find file as a registered script.\n");
        return;
    } 
    printf("Found file as fileID %d\n", (int)fileID);
    matteTable_t * t = matte_table_create_hash_pointer();
    matteArray_t * hits = matte_array_create(sizeof(uint32_t));

    printf("Traversing node graph...\n");
    uint32_t totalBytes = 0;
    uint32_t root = store->roots;
    while(root) {
        matteObjectNode_t * node = matte_pool_fetch(store->nodes, matteObjectNode_t, root);
    
        matte_store_value_object_get_memory_breakdown__find_relevant(
            store,
            t,
            hits,
            vm,
            node->data,
            fileID,
            &totalBytes
        );


        uint32_t next = node->next;        
        root = next;
    }    
    
    printf("Entire reachable set: %d KB\n", (totalBytes));

    printf("Gathering sizes...\n");
    
    
    matteArray_t * output = matte_array_create(sizeof(DataLayoutSummary));
    
    
    uint32_t i;
    uint32_t len = matte_array_get_size(hits);
    for(i = 0; i < len; ++i) {
        uint32_t next = matte_array_at(hits, uint32_t, i);
        matte_table_clear(t);
        uint32_t totalMemory = 0;
        
        matte_store_value_object_get_memory_breakdown__track_memory(
            store,
            t,
            vm,
            next,
            &totalMemory
        );
        
        DataLayoutSummary d = {};
        d.obj = matte_store_bin_fetch(store->bin, next);
        d.reachableCount = matte_table_get_size(t);
        d.estimatedWeight = totalMemory;
        d.fraction = d.estimatedWeight / totalBytes;
        
        matte_array_push(output, d);
    }
    
    qsort(
        output->data,
        output->size,
        sizeof(DataLayoutSummary),
        matte_store_value_object_get_memory_breakdown__less
    );
    
    len = matte_array_get_size(output);

    printf("Summary for snapshot of %s:\n", filename);
    printf("%7s %5s %3s %7s %10s %15s\n", 
        "ID",
        "Line",
        "Fn?", 
        "Obj#",
        "Mem (KB)",
        "% Program"
    );
    for(i = 0; i < len; ++i) {
        DataLayoutSummary d = matte_array_at(output, DataLayoutSummary, i);
        printf("%7d %5d %3s %7d %10d ",
            (int)d.obj->storeID,
            (int)d.obj->fileLine,
            (d.obj->storeID%2==0) ? "x" : " ",
            (int)d.reachableCount,
            (int)(d.estimatedWeight / 1024.0)
        );
        
        printf("[");
        uint32_t n;
        for(n = 0; n < 13; ++n) {
            if (n / 13.0 < d.fraction)
                printf("=");
            else
                printf(" ");
        }
        printf("]\n");
    }
}



#endif

#include "matte_store__gc"

