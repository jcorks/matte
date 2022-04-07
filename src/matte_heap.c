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
#include <time.h>
#ifdef MATTE_DEBUG 
    #include <assert.h>
#endif

#define ROOT_AGE_LIMIT 2


typedef struct {
    // name of the type
    matteValue_t name;
    
    // All the typecodes that this type can validly be used as in an 
    // isa comparison
    matteArray_t * isa;    
} MatteTypeData;

typedef struct matteObject_t matteObject_t;


#define VALUE_TO_HEAP(__H__, __V__) ((__V__).binID == MATTE_VALUE_TYPE_OBJECT ? (__H__)->tableHeap : (__H__)->functionHeap)
#define VALUE_PTR_TO_HEAP(__H__, __V__) ((__V__)->binID == MATTE_VALUE_TYPE_OBJECT ? (__H__)->tableHeap : (__H__)->functionHeap)

enum {
    ROOT_AGE__YOUNG,
    ROOT_AGE__OLDEST
};



struct matteHeap_t {    
    matteBin_t * tableHeap;
    matteBin_t * functionHeap;
    
    
    
    
    uint16_t gcLocked;
    int pathCheckedPool;

    // pool for value types.
    uint32_t typepool;
    uint64_t checkID;


    matteValue_t type_empty;
    matteValue_t type_boolean;
    matteValue_t type_number;
    matteValue_t type_string;
    matteValue_t type_object;
    matteValue_t type_function;
    matteValue_t type_type;
    matteValue_t type_any;
    
    
    matteTable_t * type_number_methods;
    matteTable_t * type_string_methods;
    matteTable_t * type_object_methods;
    
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
    uint32_t gcRequestStrength;
    uint32_t gcOldCycle;
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
    matteValue_t specialString_foreach;
    matteValue_t specialString_name;
    matteValue_t specialString_inherits;
    
    
    matteArray_t * toRemove;
    matteObject_t * root[ROOT_AGE__OLDEST+1];
    
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

enum {
    // whether the object is active. When inactive, the 
    // object is waiting in a bin for reuse.
    OBJECT_STATE__RECYCLED = 1,

    // whether the object is a function
    OBJECT_STATE__IS_FUNCTION = 2,

    // whether currently the object reaches a root instance
    OBJECT_STATE__REACHES_ROOT = 4,
    // whether the object is an "old" root.
    // 
    OBJECT_STATE__IS_OLD_ROOT = 8,

    // whether the object is going to be removed
    OBJECT_STATE__TO_REMOVE = 16,    
};

#define ENABLE_STATE(__o__, __s__) ((__o__)->state |= (__s__))
#define DISABLE_STATE(__o__, __s__) ((__o__)->state &= ~(__s__))
#define QUERY_STATE(__o__, __s__) (((__o__)->state & (__s__)) != 0)


struct matteObject_t{
    // any user data. Defaults to null
    void * userdata;
    
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

    // id within sorted heap.
    uint32_t heapID;
    uint16_t rootState;
    uint8_t  state;
    uint8_t  rootAge;
    uint64_t checkID;
    uint32_t oldRootLock;
    matteObject_t * prevRoot;
    matteObject_t * nextRoot;
    
    void (*nativeFinalizer)(void * objectUserdata, void * functionUserdata);
    void * nativeFinalizerData;

    
    union {
        struct {
            // keys -> matteValue_t
            matteTable_t * keyvalues_string;
            matteArray_t * keyvalues_number;
            matteTable_t * keyvalues_table;
            matteTable_t * keyvalues_functs;
            matteValue_t keyvalue_true;
            matteValue_t keyvalue_false;
            matteTable_t * keyvalues_types;

            matteValue_t attribSet;
            

            
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

};
typedef struct {
    matteObject_t * ref;
    uint32_t linkcount;
} MatteHeapParentChildLink;


static void print_list(matteObject_t * root, matteHeap_t * h) {
    matteObject_t * r = root;
    while(r) {
        printf("{%d}-", r->heapID);
        r = r->nextRoot;
    }
    printf("|\n");
}

static void push_root_node(matteHeap_t * h, matteObject_t ** root, matteObject_t * m) {
    m->nextRoot = NULL;
    m->prevRoot = NULL;
    //print_list(h);
    if (!*root)
        *root = m;
    else {
        m->nextRoot = *root;
        (*root)->prevRoot = m;
        *root = m;
    }
    //print_list(h);

}

static void remove_root_node(matteHeap_t * h, matteObject_t ** root, matteObject_t * m) {
    //print_list(h);
    if (*root == m) { // no prev
        *root = m->nextRoot;
        if (*root)
            (*root)->prevRoot = NULL;
    } else {
        if (m->prevRoot) {
            m->prevRoot->nextRoot = m->nextRoot;
        }
        
        if (m->nextRoot) {
            m->nextRoot->prevRoot = m->prevRoot;
        }
    }
    //print_list(h);
    m->nextRoot = NULL;
    m->prevRoot = NULL;

    // no longer a root. Need to remove lock status for all reachable children
    if (m->rootAge > ROOT_AGE__OLDEST) {
        uint32_t ssize = 1;
        static matteArray_t * stack = NULL;
        if (!stack) stack = matte_array_create(sizeof(matteObject_t *));
        matte_array_set_size(stack, 0);

        matte_array_push(stack, m); 
        uint32_t i, len;
        while(ssize) {
            ssize--;
            matteObject_t * o = matte_array_at(stack, matteObject_t *, ssize);

            len = matte_array_get_size(o->refChildren);
            for(i = 0; i < len; ++i) {
                matteObject_t * child = matte_array_at(o->refChildren, MatteHeapParentChildLink, i).ref;
                if (child->oldRootLock) {
                    o->oldRootLock = 0;
                    matte_array_push(stack, child);
                    ssize++;
                }
            }
        }
    }
}



static matteValue_t * object_lookup(matteHeap_t * heap, matteObject_t * m, matteValue_t key) {
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return NULL;
      case MATTE_VALUE_TYPE_STRING: {
        if (!m->table.keyvalues_string) return NULL;
        return matte_table_find_by_uint(m->table.keyvalues_string, key.value.id);
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        if (key.value.number < 0) return NULL;
        if (!m->table.keyvalues_number) return NULL;
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
      
      case MATTE_VALUE_TYPE_FUNCTION:{     
        if (!m->table.keyvalues_functs) return NULL;      
        return matte_table_find_by_uint(m->table.keyvalues_functs, key.value.id);
      }

      case MATTE_VALUE_TYPE_OBJECT: {           
        if (!m->table.keyvalues_table) return NULL;
        return matte_table_find_by_uint(m->table.keyvalues_table, key.value.id);
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        if (!m->table.keyvalues_types) return NULL;
        return matte_table_find_by_uint(m->table.keyvalues_types, key.value.id);
      }
    }
    return NULL;
}







static void object_link_parent(matteHeap_t * h, matteObject_t * parent, matteObject_t * child) {
    uint32_t i;
    uint32_t lenP = matte_array_get_size(parent->refChildren);
    uint32_t lenC = matte_array_get_size(child->refParents);


    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->ref == child) {
            iter->linkcount++;

            iter = matte_array_get_data(child->refParents);            
            for(i = 0; i < lenC; ++i, ++iter) {
                if (iter->ref == parent) {
                    iter->linkcount++;
                    return;                
                }
            }

            return;            
        }
    }
    
    if (parent->rootState && !matte_array_get_size(parent->refChildren))
        push_root_node(h, h->root+ROOT_AGE__YOUNG, parent);

    
    
    matte_array_set_size(parent->refChildren, lenP+1);
    iter = &matte_array_at(parent->refChildren, MatteHeapParentChildLink, lenP);
    iter->linkcount = 1;
    iter->ref = child;

    matte_array_set_size(child->refParents, lenC+1);
    iter = &matte_array_at(child->refParents, MatteHeapParentChildLink, lenC);
    iter->linkcount = 1;
    iter->ref = parent;
    
    
    

    //void * p = matte_table_find_by_uint(parent->refChildren, child->heapID);
    //matte_table_insert_by_uint(child->refParents,   parent->heapID, p+1);
    //matte_table_insert_by_uint(parent->refChildren, child->heapID,  p+1);
}

static void object_unlink_parent(matteHeap_t * h, matteObject_t * parent, matteObject_t * child) {
    uint32_t i, n;
    uint32_t lenP = matte_array_get_size(parent->refChildren);
    uint32_t lenC = matte_array_get_size(child->refParents);

    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->ref == child) {

            MatteHeapParentChildLink * iterO = matte_array_get_data(child->refParents);            
            for(n = 0; n < lenC; ++n, ++iterO) {
                if (iterO->ref == parent) {
                                           
                    if (iter->linkcount == 1) {
                        matte_array_remove(parent->refChildren, i);
                        matte_array_remove(child->refParents, n);
                        
                        if (!matte_array_get_size(parent->refChildren)) {
                            remove_root_node(h, h->root+(parent->rootAge <= ROOT_AGE_LIMIT ? ROOT_AGE__YOUNG : ROOT_AGE__OLDEST), parent);
                        }
                        
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

static void object_unlink_parent_child_only_all(matteHeap_t * h, matteObject_t * parent, matteObject_t * child) {
    uint32_t n;
    uint32_t lenC = matte_array_get_size(child->refParents);

    MatteHeapParentChildLink * iterO = matte_array_get_data(child->refParents);            
    for(n = 0; n < lenC; ++n, ++iterO) {
        if (iterO->ref == parent) {
                                   
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

static void object_unlink_parent_parent_only_all(matteHeap_t * h, matteObject_t * parent, matteObject_t * child) {
    uint32_t i;
    uint32_t lenP = matte_array_get_size(parent->refChildren);

    MatteHeapParentChildLink * iter = matte_array_get_data(parent->refChildren);
    for(i = 0; i < lenP; ++i, ++iter) {
        if (iter->ref == child) {                                           
            matte_array_remove(parent->refChildren, i);
            if (!matte_array_get_size(parent->refChildren)) {
                remove_root_node(h, h->root+(parent->rootAge <= ROOT_AGE_LIMIT ? ROOT_AGE__YOUNG : ROOT_AGE__OLDEST), parent);
            }
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
    object_link_parent(heap, parent,  matte_bin_fetch(VALUE_PTR_TO_HEAP(heap, child), child->value.id));
}

static void object_unlink_parent_value(matteHeap_t * heap, matteObject_t * parent, const matteValue_t * child) {
    object_unlink_parent(heap, parent,  matte_bin_fetch(VALUE_PTR_TO_HEAP(heap, child), child->value.id));
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
        if (!m->table.keyvalues_string) m->table.keyvalues_string = matte_table_create_hash_pointer();
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
        if (!m->table.keyvalues_number) m->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));
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
      
      case MATTE_VALUE_TYPE_FUNCTION: {           
        if (!m->table.keyvalues_functs) m->table.keyvalues_functs = matte_table_create_hash_pointer();
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_functs, key.value.id);
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
            matte_table_insert_by_uint(m->table.keyvalues_functs, key.value.id, value);
            return value;
        }
      }           
      case MATTE_VALUE_TYPE_OBJECT: {   
        if (!m->table.keyvalues_table) m->table.keyvalues_table = matte_table_create_hash_pointer();        
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_table, key.value.id);
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
            matte_table_insert_by_uint(m->table.keyvalues_table, key.value.id, value);
            return value;
        }
      }
      
      case MATTE_VALUE_TYPE_TYPE: {
        if (!m->table.keyvalues_types) m->table.keyvalues_types = matte_table_create_hash_pointer();
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



static void * create_table() {
    matteObject_t * out = calloc(1, sizeof(matteObject_t));
    //out->refChildren = matte_table_create_hash_pointer();
    //out->refParents = matte_table_create_hash_pointer();
    out->refChildren = matte_array_create(sizeof(MatteHeapParentChildLink));
    out->refParents = matte_array_create(sizeof(MatteHeapParentChildLink));

    out->table.keyvalues_string = NULL;//matte_table_create_hash_pointer();
    out->table.keyvalues_types  = NULL;//matte_table_create_hash_pointer();
    out->table.keyvalues_number = NULL;//matte_array_create(sizeof(matteValue_t));
    out->table.keyvalues_table = NULL;//matte_table_create_hash_pointer();
    out->table.keyvalues_functs = NULL;//matte_table_create_hash_pointer();
    out->table.keyvalue_true.binID = 0;
    out->table.keyvalue_false.binID = 0;       
    DISABLE_STATE(out, OBJECT_STATE__IS_FUNCTION);

    return out;
}

static void * create_function() {
    matteObject_t * out = calloc(1, sizeof(matteObject_t));
    //out->refChildren = matte_table_create_hash_pointer();
    //out->refParents = matte_table_create_hash_pointer();
    out->refChildren = matte_array_create(sizeof(MatteHeapParentChildLink));
    out->refParents = matte_array_create(sizeof(MatteHeapParentChildLink));

    out->function.captures = matte_array_create(sizeof(CapturedReferrable_t));
    out->function.types = NULL;
    ENABLE_STATE(out, OBJECT_STATE__IS_FUNCTION);

    return out;
}



static void destroy_object(void * d) {
    matteObject_t * out = d;

    if (QUERY_STATE(out, OBJECT_STATE__IS_FUNCTION)) {
        matte_array_destroy(out->function.captures);
    
    } else {
        matteTableIter_t * iter = matte_table_iter_create();
        if (out->table.keyvalues_string) {
            for(matte_table_iter_start(iter, out->table.keyvalues_string);
                !matte_table_iter_is_end(iter);
                matte_table_iter_proceed(iter)
            ) {
                free(matte_table_iter_get_value(iter));
            }
        }
        
        if (out->table.keyvalues_types) {
            for(matte_table_iter_start(iter, out->table.keyvalues_types);
                !matte_table_iter_is_end(iter);
                matte_table_iter_proceed(iter)
            ) {
                free(matte_table_iter_get_value(iter));
            }
        }
        
        matte_table_iter_destroy(iter);

        if (out->table.keyvalues_string) matte_table_destroy(out->table.keyvalues_string);
        if (out->table.keyvalues_types)  matte_table_destroy(out->table.keyvalues_types);
        if (out->table.keyvalues_table)  matte_table_destroy(out->table.keyvalues_table);
        if (out->table.keyvalues_functs) matte_table_destroy(out->table.keyvalues_functs);
        if (out->table.keyvalues_number) matte_array_destroy(out->table.keyvalues_number);
    }


    //matte_table_destroy(out->refParents);
    //matte_table_destroy(out->refChildren);
    matte_array_destroy(out->refParents);
    matte_array_destroy(out->refChildren);
    free(out);
}


static char * BUILTIN_NUMBER__NAMES[] = {
    "floor",
    "ceil",
    "round",
    "toRadians",
    "toDegrees",
    "PI",
    "cos",
    "sin",
    "tan",
    "acos",
    "asin",
    "atan",
    "atan2",
    "sqrt",
    "abs",
    "isNaN",
    "parse",
    "random",
    NULL
};

static int BUILTIN_NUMBER__IDS[] = {
    MATTE_EXT_CALL__NUMBER__FLOOR,
    MATTE_EXT_CALL__NUMBER__CEIL,
    MATTE_EXT_CALL__NUMBER__ROUND,
    MATTE_EXT_CALL__NUMBER__TORADIANS,
    MATTE_EXT_CALL__NUMBER__TODEGREES,
    MATTE_EXT_CALL__NUMBER__PI,
    MATTE_EXT_CALL__NUMBER__COS,
    MATTE_EXT_CALL__NUMBER__SIN,
    MATTE_EXT_CALL__NUMBER__TAN,
    MATTE_EXT_CALL__NUMBER__ACOS,
    MATTE_EXT_CALL__NUMBER__ASIN,
    MATTE_EXT_CALL__NUMBER__ATAN,
    MATTE_EXT_CALL__NUMBER__ATAN2,
    MATTE_EXT_CALL__NUMBER__SQRT,
    MATTE_EXT_CALL__NUMBER__ABS,
    MATTE_EXT_CALL__NUMBER__ISNAN,
    MATTE_EXT_CALL__NUMBER__PARSE,
    MATTE_EXT_CALL__NUMBER__RANDOM
};

static char * BUILTIN_STRING__NAMES[] = {
    "length",
    "search",
    "contains",
    "replace",
    "count",
    "charCodeAt",
    "charAt",
    "setCharCodeAt",
    "setCharAt",
    "combine",
    "removeChar",
    "substr",
    "split",
    "scan",
    NULL
};

static int BUILTIN_STRING__IDS[] = {
    MATTE_EXT_CALL__STRING__LENGTH,
    MATTE_EXT_CALL__STRING__SEARCH,
    MATTE_EXT_CALL__STRING__CONTAINS,
    MATTE_EXT_CALL__STRING__REPLACE,
    MATTE_EXT_CALL__STRING__COUNT,
    MATTE_EXT_CALL__STRING__CHARCODEAT,
    MATTE_EXT_CALL__STRING__CHARAT,
    MATTE_EXT_CALL__STRING__SETCHARCODEAT,
    MATTE_EXT_CALL__STRING__SETCHARAT,
    MATTE_EXT_CALL__STRING__COMBINE,
    MATTE_EXT_CALL__STRING__REMOVECHAR,
    MATTE_EXT_CALL__STRING__SUBSTR,
    MATTE_EXT_CALL__STRING__SPLIT,
    MATTE_EXT_CALL__STRING__SCAN
};


static char * BUILTIN_OBJECT__NAMES[] = {
    "keycount",
    "keys",
    "values",
    "length",
    "push",
    "pop",
    "insert",
    "removeKey",
    "setAttributes",
    "getAttributes",
    "sort",
    "subset",
    "filter",
    "findIndex",
    "newType",
    "instantiate",
    "is",
    "map",
    "reduce",
    NULL
};


static int BUILTIN_OBJECT__IDS[] = {
    MATTE_EXT_CALL__OBJECT__KEYCOUNT,
    MATTE_EXT_CALL__OBJECT__KEYS,
    MATTE_EXT_CALL__OBJECT__VALUES,
    MATTE_EXT_CALL__OBJECT__LENGTH,
    MATTE_EXT_CALL__OBJECT__PUSH,
    MATTE_EXT_CALL__OBJECT__POP,
    MATTE_EXT_CALL__OBJECT__INSERT,
    MATTE_EXT_CALL__OBJECT__REMOVE,
    MATTE_EXT_CALL__OBJECT__SETATTRIBUTES,
    MATTE_EXT_CALL__OBJECT__GETATTRIBUTES,
    MATTE_EXT_CALL__OBJECT__SORT,
    MATTE_EXT_CALL__OBJECT__SUBSET,
    MATTE_EXT_CALL__OBJECT__FILTER,
    MATTE_EXT_CALL__OBJECT__FINDINDEX,
    MATTE_EXT_CALL__OBJECT__NEWTYPE,
    MATTE_EXT_CALL__OBJECT__INSTANTIATE,
    MATTE_EXT_CALL__OBJECT__IS,
    MATTE_EXT_CALL__OBJECT__MAP,
    MATTE_EXT_CALL__OBJECT__REDUCE
};


static void add_table_refs(
    matteTable_t * table,
    matteStringHeap_t * stringHeap,
    char ** names,
    int * ids
) {
    while(*names) {


        
        matte_table_insert_by_uint(table, matte_string_heap_internalize_cstring(stringHeap, *names), (void*)(uintptr_t)*ids);
    
        names++;
        ids++;
    }
}




matteHeap_t * matte_heap_create(matteVM_t * vm) {
    matteHeap_t * out = calloc(1, sizeof(matteHeap_t));
    out->vm = vm;
    out->tableHeap = matte_bin_create(create_table, destroy_object);
    out->functionHeap = matte_bin_create(create_function, destroy_object);
    out->typecode2data = matte_array_create(sizeof(MatteTypeData));
    out->routePather = matte_array_create(sizeof(void*));
    out->routeIter = matte_table_iter_create();
    //out->verifiedRoot = matte_table_create_hash_pointer();
    out->stringHeap = matte_string_heap_create();
    out->checkID = 101;
    out->toRemove = matte_array_create(sizeof(matteObject_t*));

    MatteTypeData dummyD = {};
    matte_array_push(out->typecode2data, dummyD);

    out->type_empty = matte_heap_new_value(out);
    out->type_boolean = matte_heap_new_value(out);
    out->type_number = matte_heap_new_value(out);
    out->type_string = matte_heap_new_value(out);
    out->type_object = matte_heap_new_value(out);
    out->type_function = matte_heap_new_value(out);
    out->type_type = matte_heap_new_value(out);
    out->type_any = matte_heap_new_value(out);
    srand(time(NULL));
    out->type_number_methods = matte_table_create_hash_pointer();
    out->type_object_methods = matte_table_create_hash_pointer();
    out->type_string_methods = matte_table_create_hash_pointer();

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
    
    add_table_refs(out->type_number_methods, out->stringHeap, BUILTIN_NUMBER__NAMES, BUILTIN_NUMBER__IDS);
    add_table_refs(out->type_object_methods, out->stringHeap, BUILTIN_OBJECT__NAMES, BUILTIN_OBJECT__IDS);
    add_table_refs(out->type_string_methods, out->stringHeap, BUILTIN_STRING__NAMES, BUILTIN_STRING__IDS);

    return out;
    
}

void matte_heap_destroy(matteHeap_t * h) {

    h->shutdown = 1;
    while(matte_array_get_size(h->toRemove)) {
        h->cooldown = 0;
        matte_heap_garbage_collect(h);
    }


    #ifdef MATTE_DEBUG__HEAP
    assert(matte_heap_report(h) == 0);
    #endif

    matte_bin_destroy(h->tableHeap);
    matte_bin_destroy(h->functionHeap);
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
    matte_table_destroy(h->type_number_methods);
    matte_table_destroy(h->type_object_methods);
    matte_table_destroy(h->type_string_methods);
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



void matte_value_into_new_object_ref_(matteHeap_t * heap, matteValue_t * v) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->tableHeap, &v->value.id);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
}


void matte_value_into_new_object_ref_typed_(matteHeap_t * heap, matteValue_t * v, matteValue_t typeobj) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->tableHeap, &v->value.id);
    d->heapID = v->value.id;
    if (typeobj.binID != MATTE_VALUE_TYPE_TYPE) {        
        matteString_t * str = matte_string_create_from_c_str("Cannot instantiate object without a Type. (given value is of type %s)", matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, typeobj))));
        matte_vm_raise_error_string(heap->vm, str);
        matte_string_destroy(str);
        d->table.typecode = heap->type_object.value.id;
    } else {
        d->table.typecode = typeobj.value.id;
    }
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
}


void matte_value_into_new_object_literal_ref_(matteHeap_t * heap, matteValue_t * v, const matteArray_t * arr) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->tableHeap, &v->value.id);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
    matte_value_object_push_lock(heap, *v);
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    
    for(i = 0; i < len; i+=2) {
        matteValue_t key   = matte_array_at(arr, matteValue_t, i);
        matteValue_t value = matte_array_at(arr, matteValue_t, i+1);
        
        object_put_prop(heap, d, key, value);
    }

    matte_value_object_pop_lock(heap, *v);


}


void matte_value_into_new_object_array_ref_(matteHeap_t * heap, matteValue_t * v, const matteArray_t * args) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(heap->tableHeap, &v->value.id);
    d->heapID = v->value.id;
    d->table.typecode = heap->type_object.value.id;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);
    matte_value_object_push_lock(heap, *v);

    uint32_t i;
    uint32_t len = matte_array_get_size(args);

    if (!d->table.keyvalues_number) d->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));
    matte_array_set_size(d->table.keyvalues_number, len);
    for(i = 0; i < len; ++i) {
        matteValue_t val = matte_heap_new_value(heap);
        matte_value_into_copy(heap, &val, matte_array_at(args, matteValue_t, i));
        matte_array_at(d->table.keyvalues_number, matteValue_t, i) = val;
        if (val.binID == MATTE_VALUE_TYPE_OBJECT || val.binID == MATTE_VALUE_TYPE_FUNCTION) {
            object_link_parent_value(heap, d, &val);
        }
    }

    matte_value_object_pop_lock(heap, *v);

}

matteValue_t matte_value_frame_get_named_referrable(matteHeap_t * heap, matteVMStackFrame_t * vframe, matteValue_t name) {
    if (vframe->context.binID != MATTE_VALUE_TYPE_FUNCTION) return matte_heap_new_value(heap);
    matteObject_t * m = matte_bin_fetch(heap->functionHeap, vframe->context.value.id);
    if (!m->function.stub) return matte_heap_new_value(heap);
    // claim captures immediately.
    uint32_t i;
    uint32_t len;



    // referrables come from a history of creation contexts.
    matteValue_t context = vframe->context;
    matteValue_t contextReferrable = vframe->referrable;

    matteValue_t out = matte_heap_new_value(heap);

    while(context.binID) {
        matteObject_t * origin = matte_bin_fetch(heap->functionHeap, context.value.id);
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
                str = matte_string_create_from_c_str("Function constructor with type specifiers requires those specifications to be Types. The type specifier for Argument '%s' is not a Type.\n",                 matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_bytecode_stub_get_arg_name(stub, i))));
            }
            
            matte_vm_raise_error_string(heap->vm, str);
            matte_string_destroy(str);
            return;
        }
    }
    
    matte_value_into_new_function_ref(heap, v, stub);
    matteObject_t * d = matte_bin_fetch(heap->functionHeap, v->value.id);
    d->function.types = matte_array_clone(args);
}

void matte_value_into_new_function_ref_(matteHeap_t * heap, matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_heap_recycle(heap, *v);
    v->binID = MATTE_VALUE_TYPE_FUNCTION;
    matteObject_t * d = matte_bin_add(heap->functionHeap, &v->value.id);
    d->heapID = v->value.id;
    d->function.stub = stub;
    DISABLE_STATE(d, OBJECT_STATE__RECYCLED);



    // claim captures immediately.
    uint32_t i;
    uint32_t len;
    const matteBytecodeStubCapture_t * capturesRaw = matte_bytecode_stub_get_captures(
        stub,
        &len
    );
    matteVMStackFrame_t frame;

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
            matteObject_t * origin = matte_bin_fetch(heap->functionHeap, context.value.id);
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
    matteObject_t * d = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
        return m->function.stub;
    }
    return NULL;
}

matteValue_t * matte_value_get_captured_value(matteHeap_t * heap, matteValue_t v, uint32_t index) {
    if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
        if (index >= matte_array_get_size(m->function.captures)) return NULL;
        CapturedReferrable_t referrable = matte_array_at(m->function.captures, CapturedReferrable_t, index);
        return matte_value_object_array_at_unsafe(heap, referrable.referrableSrc, referrable.index);
    }
    return NULL;
}

void matte_value_set_captured_value(matteHeap_t * heap, matteValue_t v, uint32_t index, matteValue_t val) {
    if (v.binID == MATTE_VALUE_TYPE_FUNCTION) {
        matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
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
    matteObject_t * m =  matte_bin_fetch(h->tableHeap, id);
    if (!m) {
        printf("{cannot print children: is not table?}\n");
        return;
    }

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
        uint32_t nc = matte_array_at(m->refChildren, MatteHeapParentChildLink, i).ref->heapID;
        printf("  {%d}", nc);
        printf("\n");

    }
    //matte_table_iter_destroy(iter);
}

void matte_heap_object_print_parents(matteHeap_t * h, uint32_t id) {
    matteObject_t * m =  matte_bin_fetch(h->tableHeap, id);
    if (!m) {
        printf("{cannot print parents: is not table?}\n");
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

        uint32_t nc = matte_array_at(m->refParents, MatteHeapParentChildLink, i).ref->heapID;
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
        matteObject_t * m = matte_bin_fetch(VALUE_TO_HEAP(heap, v), v.value.id);            
        if (!m) {
            printf("  (WARNING: this refers to an object fully recycled.)\n");
            fflush(stdout);
            return;
        }
        if (QUERY_STATE(m, OBJECT_STATE__RECYCLED)) {
            printf("  (WARNING: this object is currently dormant.)\n") ;          
        }
        printf("  RootLock:  %d\n", m->rootState);
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
    if (v.binID != MATTE_VALUE_TYPE_OBJECT && v.binID != MATTE_VALUE_TYPE_FUNCTION) {
        matte_vm_raise_error_cstring(heap->vm, "Tried to set native finalizer on a non-object/non-function value.");
        return;
    }
    matteObject_t * m = matte_bin_fetch(VALUE_TO_HEAP(heap, v), v.value.id);
    m->nativeFinalizer = fb;
    m->nativeFinalizerData = functionUserdata;        
}


void matte_value_object_set_userdata(matteHeap_t * heap, matteValue_t v, void * userData) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
        m->userdata = userData;
    }
}

void * matte_value_object_get_userdata(matteHeap_t * heap, matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
    matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
    return 1 + (m->function.types != NULL); 
}

int matte_value_object_function_pre_typecheck_unsafe(matteHeap_t * heap, matteValue_t v, const matteArray_t * arr) {
    matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
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
                "Argument '%s' (type: %s) is not of required type %s",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_bytecode_stub_get_arg_name(m->function.stub, i))),
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
    matteObject_t * m = matte_bin_fetch(heap->functionHeap, v.value.id);
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
    switch(v.binID) {
      case MATTE_VALUE_TYPE_TYPE: {// built-in functions
        matteValue_t * b = matte_value_object_access_direct(heap, v, key, isBracketAccess);
        if (b) return *b;
        return matte_heap_new_value(heap);        
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
      
      default: {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, v))))
        );
        matte_vm_raise_error_string(heap->vm, err);
        return matte_heap_new_value(heap);
      }
      
    }


}


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t * matte_value_object_access_direct(matteHeap_t * heap, matteValue_t v, matteValue_t key, int isBracketAccess) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_TYPE: {
        if (isBracketAccess) {
            matteString_t * err = matte_string_create_from_c_str(
                "Types can only yield access to built-in functions through the dot '.' accessor."
            );
            matte_vm_raise_error_string(heap->vm, err);
            return NULL;
        }
        if (key.binID != MATTE_VALUE_TYPE_STRING) {
            matteString_t * err = matte_string_create_from_c_str(
                "Types can only yield access to built-in functions through the string keys."
            );
            matte_vm_raise_error_string(heap->vm, err);
            return NULL;
        }
        
        matteTable_t * base = NULL;
        switch(v.value.id) {
          case 3: base = heap->type_number_methods; break;            
          case 4: base = heap->type_string_methods; break;
          case 5: base = heap->type_object_methods; break;
          default:;
        };
        
        if (!base) {
            matteString_t * err = matte_string_create_from_c_str(
                "The given type has no built-in functions. (Given value is of type: %s)",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, v))))
            );
            matte_vm_raise_error_string(heap->vm, err);
            return NULL;
        }
        
        int out = (uintptr_t)matte_table_find_by_uint(base, key.value.id);
        // for Types, its an error if no function is found.
        if (!out) {
            matteString_t * err = matte_string_create_from_c_str(
                "The given type has no built-in function by the name '%s'",
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, key))
            );
            matte_vm_raise_error_string(heap->vm, err);
            return NULL;
        }
        return matte_vm_get_external_builtin_function_as_value(heap->vm, out);
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
        matteValue_t accessor = object_get_access_operator(heap, m, isBracketAccess, 1);
        if (accessor.binID) {
            return NULL;
        } else {
            return object_lookup(heap, m, key);
        }      
      }

      default: {
        matteString_t * err = matte_string_create_from_c_str(
            "Cannot access member of a non-object type. (Given value is of type: %s)",
            matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_type_name(heap, matte_value_get_type(heap, v))))
        );
        matte_vm_raise_error_string(heap->vm, err);
        return NULL; //matte_heap_new_value(heap);
      
      
      }
        
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
    matteObject_t * m = matte_bin_fetch(VALUE_TO_HEAP(heap, v), v.value.id);
    if (m->rootState == 0) {
        if (matte_array_get_size(m->refChildren))
            push_root_node(heap, heap->root+ROOT_AGE__YOUNG, m);
    }
    m->rootState++;
    #ifdef MATTE_DEBUG__HEAP    
    printf("%d + at state %d\n", v.value.id, m->rootState);
    #endif
}

void matte_value_object_pop_lock_(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT && v.binID != MATTE_VALUE_TYPE_FUNCTION) return;
    matteObject_t * m = matte_bin_fetch(VALUE_TO_HEAP(heap, v), v.value.id);
    if (m->rootState) {
        m->rootState--;
        if (m->rootState == 0) {
            if (matte_array_get_size(m->refChildren)) {
                remove_root_node(heap, heap->root+(m->rootAge <= ROOT_AGE_LIMIT ? ROOT_AGE__YOUNG : ROOT_AGE__OLDEST), m);        
            }
            matte_heap_recycle(heap, v);
            heap->gcRequestStrength++;
        }
    }     
    #ifdef MATTE_DEBUG__HEAP    
    printf("%d - at state %d\n", v.value.id, m->rootState);
    #endif
}



matteValue_t matte_value_object_keys(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(heap);
    }
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
    if (len) {
        for(i = 0; i < len; ++i) {
            matteValue_t key = matte_heap_new_value(heap);
            matte_value_into_number(heap, &key, i);
            matte_array_push(keys, key);
        }
    }

    // then string
    if (m->table.keyvalues_string && !matte_table_is_empty(m->table.keyvalues_string)) {
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
    if (m->table.keyvalues_table && !matte_table_is_empty(m->table.keyvalues_table)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_table);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key;
            matteObject_t * current = matte_bin_fetch(heap->tableHeap, matte_table_iter_get_key_uint(iter));
            key.binID = MATTE_VALUE_TYPE_OBJECT;
            key.value.id = current->heapID;
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_in(heap, key, "object.keys()", 0);
            #endif
            matte_array_push(keys, key);    
        }
        matte_table_iter_destroy(iter);
    }

    if (m->table.keyvalues_functs && !matte_table_is_empty(m->table.keyvalues_functs)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_functs);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            matteValue_t key;
            matteObject_t * current = matte_bin_fetch(heap->functionHeap, matte_table_iter_get_key_uint(iter));
            key.binID = MATTE_VALUE_TYPE_FUNCTION;
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
    if (m->table.keyvalues_types && !matte_table_is_empty(m->table.keyvalues_types)) {
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
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    matteArray_t * vals = matte_array_create(sizeof(matteValue_t));
    matteValue_t val;
    // first number 
    uint32_t i;
    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
    if (len) {
        for(i = 0; i < len; ++i) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, matte_array_at(m->table.keyvalues_number, matteValue_t, i));
            matte_array_push(vals, val);
        }
    }

    // then string
    if (m->table.keyvalues_string && !matte_table_is_empty(m->table.keyvalues_string)) {
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
    if (m->table.keyvalues_table && !matte_table_is_empty(m->table.keyvalues_table)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_table);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            val = matte_heap_new_value(heap);
            matte_value_into_copy(heap, &val, *(matteValue_t *)matte_table_iter_get_value(iter));
            matte_array_push(vals, val);    
        }
        matte_table_iter_destroy(iter);
    }
    if (m->table.keyvalues_functs && !matte_table_is_empty(m->table.keyvalues_functs)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_functs);
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
    if (m->table.keyvalues_types && !matte_table_is_empty(m->table.keyvalues_types)) {
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

// Returns the number of number keys within the object, ignoring keys of other types.
uint32_t matte_value_object_get_number_key_count(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return 0;
    }
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    return m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
}


uint32_t matte_value_object_get_key_count(matteHeap_t * heap, matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return 0;
    }
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    uint32_t total = 
        (m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0) +
        (m->table.keyvalue_true.binID?1:0) +
        (m->table.keyvalue_false.binID?1:0);

    // maybe maintain through get/set/remove prop?
    if (m->table.keyvalues_string && !matte_table_is_empty(m->table.keyvalues_string)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }


    if (m->table.keyvalues_table && !matte_table_is_empty(m->table.keyvalues_table)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_table);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }

    if (m->table.keyvalues_functs && !matte_table_is_empty(m->table.keyvalues_functs)) {
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_functs);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            total++;
        }
        matte_table_iter_destroy(iter);
    }

    
    if (m->table.keyvalues_types && !matte_table_is_empty(m->table.keyvalues_types)) {
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
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return;
      case MATTE_VALUE_TYPE_STRING: {
        if (!m->table.keyvalues_string) return;
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
        if (!m->table.keyvalues_types) return;
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
        if (!m->table.keyvalues_number) return;
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
      
      case MATTE_VALUE_TYPE_FUNCTION: {
        if (!m->table.keyvalues_functs) return;           
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_functs, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);                
            }
            object_link_parent_value(heap, m, &key);
            matte_heap_recycle(heap, *value);
            matte_table_remove_by_uint(m->table.keyvalues_functs, key.value.id);
        }
        return;
      }          
      case MATTE_VALUE_TYPE_OBJECT: {           
        if (!m->table.keyvalues_table) return;
        matteValue_t * value = matte_table_find_by_uint(m->table.keyvalues_table, key.value.id);
        if (value) {
            if (value->binID == MATTE_VALUE_TYPE_OBJECT || value->binID == MATTE_VALUE_TYPE_FUNCTION) {
                object_unlink_parent_value(heap, m, value);                
            }
            object_link_parent_value(heap, m, &key);
            matte_heap_recycle(heap, *value);
            matte_table_remove_by_uint(m->table.keyvalues_table, key.value.id);
        }
        return;
      }
    }

}

void matte_value_object_foreach(matteHeap_t * heap, matteValue_t v, matteValue_t func) {
    // 
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);

    // foreach operator
    if (m->table.attribSet.binID) {
        matteValue_t set = matte_value_object_access(heap, m->table.attribSet, heap->specialString_foreach, 0);

        if (set.binID) {
            v = matte_vm_call(heap->vm, set, matte_array_empty(), matte_array_empty(), NULL);
            if (v.binID != MATTE_VALUE_TYPE_FUNCTION) {
                matte_vm_raise_error_string(heap->vm, MATTE_VM_STR_CAST(heap->vm, "foreach attribute MUST return a function."));
                return;
            } else {
                m = matte_bin_fetch(heap->functionHeap, v.value.id);
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

    matteArray_t *keys   = matte_array_create(sizeof(matteValue_t));
    matteArray_t *values = matte_array_create(sizeof(matteValue_t));

    // first number 
    uint32_t i;
    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
    if (len) {
        matte_value_into_number(heap, &args[0], 0);
        double * quickI = &args[0].value.number;

        for(i = 0; i < len; ++i) {
            args[1] = matte_array_at(m->table.keyvalues_number, matteValue_t, i);
            *quickI = i;
            matte_value_object_push_lock(heap, args[1]);
            matte_array_push(keys, args[0]);
            matte_array_push(values, args[1]);
        }
        args[0].binID = 0;
    }

    // then string
    if (m->table.keyvalues_string && !matte_table_is_empty(m->table.keyvalues_string)) {
        matteString_t * e = matte_string_create();
        matte_value_into_string(heap, &args[0], e);
        matte_string_destroy(e);
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_string);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matte_value_object_push_lock(heap, args[1]);
            matte_array_push(keys, args[0]);
            matte_array_push(values, args[1]);

        }
        matte_table_iter_destroy(iter);
    }


    // object 
    if (m->table.keyvalues_table && !matte_table_is_empty(m->table.keyvalues_table)) {
        args[0].binID = MATTE_VALUE_TYPE_OBJECT;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_table);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matte_value_object_push_lock(heap, args[1]);
            matte_value_object_push_lock(heap, args[0]);
            matte_array_push(keys, args[0]);
            matte_array_push(values, args[1]);


        }
        matte_table_iter_destroy(iter);
    }
    if (m->table.keyvalues_functs && !matte_table_is_empty(m->table.keyvalues_functs)) {
        args[0].binID = MATTE_VALUE_TYPE_FUNCTION;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_functs);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matte_value_object_push_lock(heap, args[1]);
            matte_value_object_push_lock(heap, args[0]);
            matte_array_push(keys, args[0]);
            matte_array_push(values, args[1]);


        }
        matte_table_iter_destroy(iter);
    }
    // true 
    if (m->table.keyvalue_true.binID) {
        matte_value_into_boolean(heap, &args[0], 1);
        args[1] = m->table.keyvalue_true;
        matte_value_object_push_lock(heap, args[1]);
        matte_array_push(keys, args[0]);
        matte_array_push(values, args[1]);
    }
    // false
    if (m->table.keyvalue_false.binID) {
        matte_value_into_boolean(heap, &args[0], 1);
        args[1] = m->table.keyvalue_false;
        matte_value_object_push_lock(heap, args[1]);
        matte_array_push(keys, args[0]);
        matte_array_push(values, args[1]);
    }

    if (m->table.keyvalues_types && !matte_table_is_empty(m->table.keyvalues_types)) {
        matte_heap_recycle(heap, args[0]);
        args[0].binID = MATTE_VALUE_TYPE_TYPE;
        matteTableIter_t * iter = matte_table_iter_create();
        matte_table_iter_start(iter, m->table.keyvalues_types);
        for(; !matte_table_iter_is_end(iter); matte_table_iter_proceed(iter)) {
            args[1] = *(matteValue_t*)matte_table_iter_get_value(iter);
            args[0].value.id = matte_table_iter_get_key_uint(iter);
            matte_value_object_push_lock(heap, args[1]);
            matte_array_push(keys, args[0]);
            matte_array_push(values, args[1]);

        }
        //matte_heap_recycle(heap, args[0]);
        args[0].binID = 0;
        matte_table_iter_destroy(iter);
    }
    
    
    // now we can safely iterate
    len = matte_array_get_size(keys);
    for(i = 0; i < len; ++i) {
        args[0] = matte_array_at(keys, matteValue_t, i);
        args[1] = matte_array_at(values, matteValue_t, i);

        matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
        matteValue_t r = matte_vm_call(heap->vm, func, &arr, &argNames_array, NULL);
        matte_heap_recycle(heap, r);
    
        matte_value_object_pop_lock(heap, args[0]);
        matte_value_object_pop_lock(heap, args[1]);
    }
    
    matte_array_destroy(keys);
    matte_array_destroy(values);
    
    
    matte_value_object_pop_lock(heap, v);

}

matteValue_t matte_value_subset(matteHeap_t * heap, matteValue_t v, uint32_t from, uint32_t to) {
    matteValue_t out = matte_heap_new_value(heap);
    if (from > to) return out;

    switch(v.binID) {
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);


        uint32_t curlen = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
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
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);

    matteString_t * str = matte_string_create();
    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
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
    
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    if (m->table.attribSet.binID) {
        object_unlink_parent_value(heap, m, &m->table.attribSet);
        matte_heap_recycle(heap, m->table.attribSet);
    }

    
    matte_value_into_copy(heap, &m->table.attribSet, opObject);
    object_link_parent_value(heap, m, &m->table.attribSet);
}

const matteValue_t * matte_value_object_get_attributes_unsafe(matteHeap_t * heap, matteValue_t v) {
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
    if (m->table.attribSet.binID) return &m->table.attribSet;
    return NULL;
}



typedef struct {
    matteValue_t fn;
    matteArray_t names;
    matteHeap_t * heap;
} matteParams_Sort_t;

typedef struct {
    matteValue_t value;
    matteParams_Sort_t * sort;
} matteValue_Sort_t;  

static int matte_value_object_sort__cmp(
    const void * asrc,
    const void * bsrc
) {
    matteValue_Sort_t * a = (*(matteValue_Sort_t **)asrc);
    matteValue_Sort_t * b = (*(matteValue_Sort_t **)bsrc);

    matteValue_t args[] = {
        (*(matteValue_Sort_t **)a)->value,
        b->value
    };
    matteArray_t arr = MATTE_ARRAY_CAST(args, matteValue_t, 2);
    return matte_value_as_number(a->sort->heap, matte_vm_call(a->sort->heap->vm, a->sort->fn, &arr, &a->sort->names, NULL));    
}


void matte_value_object_sort_unsafe(matteHeap_t * heap, matteValue_t v, matteValue_t less) {


    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);

    uint32_t len = m->table.keyvalues_number ? matte_array_get_size(m->table.keyvalues_number) : 0;
    if (len < 1) return;

    matteParams_Sort_t params;
    params.fn = less;
    params.heap = heap;
    
    matte_value_object_push_lock(heap, v);
    matte_value_object_push_lock(heap, less);

    matteValue_t names[2];
    names[0].binID = MATTE_VALUE_TYPE_STRING;
    names[1].binID = MATTE_VALUE_TYPE_STRING;
    names[0].value.id = matte_string_heap_internalize_cstring(heap->stringHeap, "a");
    names[1].value.id = matte_string_heap_internalize_cstring(heap->stringHeap, "b");
    params.names = MATTE_ARRAY_CAST(names, matteValue_t, 2);
    
    matteValue_Sort_t * sortArray = malloc(sizeof(matteValue_Sort_t)*len);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        sortArray[i].value = matte_array_at(m->table.keyvalues_number, matteValue_t, i);
        sortArray[i].sort = &params;
    }
    
    qsort(sortArray, len, sizeof(matteValue_Sort_t), matte_value_object_sort__cmp);
    
    for(i = 0; i < len; ++i) {
        matte_array_at(m->table.keyvalues_number, matteValue_t, i) = sortArray[i].value;
    }
    free(sortArray);
    
    matte_value_object_pop_lock(heap, v);
    matte_value_object_pop_lock(heap, less);

   
}



void matte_value_object_insert(
    matteHeap_t * heap, 
    matteValue_t v, 
    uint32_t index, 
    matteValue_t val
) {
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);

    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return;
    }

    if (val.binID == MATTE_VALUE_TYPE_OBJECT || val.binID == MATTE_VALUE_TYPE_FUNCTION) {    
        object_link_parent_value(heap, m, &val);
    }
    if (!m->table.keyvalues_number) m->table.keyvalues_number = matte_array_create(sizeof(matteValue_t));
    matte_array_insert(
        m->table.keyvalues_number,
        index,
        val
    );
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
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
    matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(heap->tableHeap, v.value.id);
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
        matteObject_t * m = matte_bin_fetch(VALUE_TO_HEAP(heap, v), v.value.id);
        if (!m) return;
        if (!QUERY_STATE(m, OBJECT_STATE__TO_REMOVE) && !m->rootState) {
            matte_array_push(heap->toRemove, m);
            ENABLE_STATE(m, OBJECT_STATE__TO_REMOVE);
        }
    } 
    
}



static void print_state_bits(matteObject_t * m) {
    printf(
        "OBJECT_STATE__IS_FUNCTION: %d\n"
        "OBJECT_STATE__IS_OLD_ROOT: %d\n"
        "OBJECT_STATE__REACHES_ROOT:%d\n"
        "OBJECT_STATE__RECYCLED:    %d\n"
        "OBJECT_STATE__TO_REMOVE:   %d\n",

        QUERY_STATE(m, OBJECT_STATE__IS_FUNCTION),
        QUERY_STATE(m, OBJECT_STATE__IS_OLD_ROOT),
        QUERY_STATE(m, OBJECT_STATE__REACHES_ROOT),
        QUERY_STATE(m, OBJECT_STATE__RECYCLED),
        QUERY_STATE(m, OBJECT_STATE__TO_REMOVE)
    );        
}



static void object_cleanup(matteHeap_t * h) {
    matteTableIter_t * iter = matte_table_iter_create();
    if (!matte_array_get_size(h->toRemove)) return;


    matteArray_t * toRemove = h->toRemove;
    // preserve current size. Real size may increase midway through 
    // but only object_cleanup will reduce the size (end of function);
    int cleanedUP = 0;
    uint32_t len = matte_array_get_size(h->toRemove);

    uint32_t i;
    for(i = 0; i < len; ++i) {
        matteObject_t * m = matte_array_at(toRemove, matteObject_t *, i);
        DISABLE_STATE(m, OBJECT_STATE__TO_REMOVE);
        if (m->checkID == h->checkID && QUERY_STATE(m, OBJECT_STATE__REACHES_ROOT)) continue;
        if (m->rootState || m->oldRootLock) continue;
        if (QUERY_STATE(m, OBJECT_STATE__RECYCLED)) continue;
        cleanedUP++;
        #ifdef MATTE_DEBUG__HEAP

            printf("--RECYCLING OBJECT %d\n", m->heapID);

            {
                matteValue_t vl;
                vl.binID = QUERY_STATE(m, OBJECT_STATE__IS_FUNCTION) ? MATTE_VALUE_TYPE_FUNCTION : MATTE_VALUE_TYPE_OBJECT;
                vl.value.id = m->heapID;
                matte_heap_track_out(h, vl, "<removal>", 0);
            }
        #endif


        ENABLE_STATE(m, OBJECT_STATE__RECYCLED);

        if (matte_array_get_size(m->refChildren)) {
            matteValue_t v;
            v.binID = MATTE_VALUE_TYPE_OBJECT;

            uint32_t i;
            uint32_t len = matte_array_get_size(m->refChildren);
            MatteHeapParentChildLink * iter = matte_array_get_data(m->refChildren);
            for(i = 0; i < len; ++i , ++iter) {
                matteObject_t * ch = iter->ref;
                if (ch) {
                    object_unlink_parent_child_only_all(h, m, ch);
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
                matteObject_t * ch = iter->ref;
                if (ch) {
                    object_unlink_parent_parent_only_all(h, ch, m);
                    matte_heap_recycle(h, v);
                }
            }
            matte_array_set_size(m->refParents, 0);
        }



        if (QUERY_STATE(m, OBJECT_STATE__IS_FUNCTION)) {
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
            matte_bin_recycle(h->functionHeap, m->heapID);
        
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
            if (m->table.keyvalues_number && matte_array_get_size(m->table.keyvalues_number)) {
                uint32_t n;
                uint32_t subl = matte_array_get_size(m->table.keyvalues_number);
                for(n = 0; n < subl; ++n) {
                    matte_heap_recycle(h, matte_array_at(m->table.keyvalues_number, matteValue_t, n));
                }
                matte_array_set_size(m->table.keyvalues_number, 0);
            }

            if (m->table.keyvalues_string && !matte_table_is_empty(m->table.keyvalues_string)) {
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
            
            if (m->table.keyvalues_types && !matte_table_is_empty(m->table.keyvalues_types)) {

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
            if (m->table.keyvalues_table && !matte_table_is_empty(m->table.keyvalues_table)) {
                for(matte_table_iter_start(iter, m->table.keyvalues_table);
                    matte_table_iter_is_end(iter) == 0;
                    matte_table_iter_proceed(iter)
                ) {
                    matteValue_t * v = matte_table_iter_get_value(iter);
                    matte_heap_recycle(h, *v);
                    free(v);
                }
                matte_table_clear(m->table.keyvalues_table);
            }
            
            if (m->table.keyvalues_functs && !matte_table_is_empty(m->table.keyvalues_functs)) {
                for(matte_table_iter_start(iter, m->table.keyvalues_functs);
                    matte_table_iter_is_end(iter) == 0;
                    matte_table_iter_proceed(iter)
                ) {
                    matteValue_t * v = matte_table_iter_get_value(iter);
                    matte_heap_recycle(h, *v);
                    free(v);
                }
                matte_table_clear(m->table.keyvalues_functs);
            }
            matte_bin_recycle(h->tableHeap, m->heapID);
        }
        if (m->nativeFinalizer) {
            m->nativeFinalizer(m->userdata, m->nativeFinalizerData);
            m->nativeFinalizer = NULL;
            m->nativeFinalizerData = NULL;
        }
        // clean up object;
        m->userdata = NULL;
        m->rootState = 0;
        DISABLE_STATE(m, OBJECT_STATE__TO_REMOVE);        
        DISABLE_STATE(m, OBJECT_STATE__IS_OLD_ROOT);        
        DISABLE_STATE(m, OBJECT_STATE__REACHES_ROOT);        
        m->prevRoot = NULL;
        m->nextRoot = NULL;
        m->rootAge  = 0;
        
    }
    printf("cleaned Up: %d\n", cleanedUP);
    matte_table_iter_destroy(iter);
    uint32_t newlen = matte_array_get_size(h->toRemove);
    if (newlen != len) {
        memmove(
            matte_array_get_data(h->toRemove),
            &matte_array_at(h->toRemove, matteObject_t *, len),
            (newlen - len)*sizeof(matteObject_t*)
        );
        matte_array_set_size(h->toRemove, newlen - len);
    } else {
        matte_array_set_size(h->toRemove, 0);    
    }  
}

void matte_heap_push_lock_gc(matteHeap_t * h) {
    h->gcLocked++;
}
void matte_heap_pop_lock_gc(matteHeap_t * h) {
    h->gcLocked--;
}

#define UNOWNED_COUNT_COLLECT_GARBAGE 10000
#define OLD_CYCLE_COUNT_LIMIT   7

void check_roots(matteHeap_t * h, matteObject_t ** rootSrc) {
    if (!*rootSrc) return;
    static matteArray_t * stack = NULL;
    if (!stack) stack = matte_array_create(sizeof(matteObject_t*));
    else matte_array_set_size(stack, 0);
    uint32_t i;
    uint32_t len;

    matteObject_t * root = *rootSrc;        
    uint32_t roots = 0;
    int checked = 0;
    while(root) {
        roots++;
        
        uint32_t ssize = 1;
        
        root->checkID = h->checkID;
        if (root->rootAge < 0xff)
            root->rootAge++;
        
        if (root->rootAge > ROOT_AGE_LIMIT) {   
            root = root->nextRoot;
            remove_root_node(h, h->root+ROOT_AGE__YOUNG, root);
            push_root_node(h, h->root+ROOT_AGE__OLDEST, root);
            continue;
        }
        matte_array_push(stack, root);


        while(ssize) {
            ssize--;
            matteObject_t * obj = matte_array_at(stack, matteObject_t *, ssize);            
            matte_array_set_size(stack, ssize);
                            
            ENABLE_STATE(obj, OBJECT_STATE__REACHES_ROOT);
            if (*rootSrc == h->root[ROOT_AGE__OLDEST]) {
                obj->oldRootLock = 1;
            }
            checked++;


            len = matte_array_get_size(obj->refChildren);
            for(i = 0; i < len; ++i) {
                MatteHeapParentChildLink * link = &matte_array_at(obj->refChildren, MatteHeapParentChildLink, i);
                if (link->ref->checkID != h->checkID && !link->ref->oldRootLock) {
                    link->ref->checkID = h->checkID;
                    matte_array_push(stack, link->ref);
                    ssize++;
                }
            }
        }
        matte_array_set_size(stack, 0);
        root = root->nextRoot;
    }
    printf("Roots: %d, checked %d\n", roots, checked);

}

void matte_heap_garbage_collect(matteHeap_t * h) {
    if (h->gcLocked) return;

    if (!h->shutdown && h->gcRequestStrength < UNOWNED_COUNT_COLLECT_GARBAGE) return;
    h->gcLocked = 1;
    h->gcRequestStrength = 0;
    
    // Generational GC:
    // first cycle across the oldest roots occasionally
    if (h->gcOldCycle > OLD_CYCLE_COUNT_LIMIT) {
        h->gcOldCycle = 0;
        check_roots(h, h->root+ROOT_AGE__OLDEST);
    }

    h->checkID++;
    check_roots(h, h->root+ROOT_AGE__YOUNG);
    object_cleanup(h);    
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

matteValue_t matte_heap_track_in_lock(matteHeap_t * heap, matteValue_t val, const char * file, int line) {
    matteString_t * st = matte_string_create_from_c_str("%s (LOCK PUSHED)", file);
    matteValue_t out = matte_heap_track_in(heap, val, matte_string_get_c_str(st), line);
    matte_string_destroy(st);
    return out;
}

void matte_heap_track_out_lock(matteHeap_t * heap, matteValue_t val, const char * file, int line) {
    matteString_t * st = matte_string_create_from_c_str("%s (LOCK POPPED)", file);
    matte_heap_track_out(heap, val, matte_string_get_c_str(st), line);
    matte_string_destroy(st);
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
