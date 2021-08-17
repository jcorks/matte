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
struct matteHeap_t {    
    // 1 -> number. Each pointer is just a double
    // 2 -> boolean 
    // 3 -> string 
    // 4 -> object
    matteBin_t * sortedHeaps[5];
    
    
    
    
    // objects with suspected ref count 0.
    // needs a double check before confirming.
    // matteObject_t *
    matteTable_t * toSweep;
    
    int gcLocked;

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



    // has only keyvalues_number members
    int isArrayLike;
    
    
    // stub for functions. If null, is a normal object.
    matteBytecodeStub_t * functionstub;

    // call captured variables. Defined at function creation.
    matteArray_t * functionCaptures;

    // for function objects, a reference to the original 
    // creation context is kept. This is because referrables 
    // can track back to original creation contexts.
    matteValue_t origin;
    matteValue_t origin_referrable;
    // reference count
    uint32_t refs;
    
    // id within sorted heap.
    uint32_t heapID;
} matteObject_t;


static matteValue_t object_lookup(matteObject_t * m, matteValue_t key) {
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
    }
}


static void object_put_prop(matteObject_t * m, matteValue_t key, matteValue_t val) {
    matteValue_t out = matte_heap_new_value(val.heap);
    matte_value_into_copy(&out, val);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return; // SHOULD NEVER ENTER. if it would, memory leak.
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue_t * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            matte_heap_recycle(*value);
            *value = out;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert(m->keyvalues_string, str, value);
        }
        return;
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
        matte_heap_recycle(matte_array_at(m->keyvalues_number, matteValue_t, index));
        matte_array_at(m->keyvalues_number, matteValue_t, index) = out;
        return;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);

        if (*d) {
            matte_heap_recycle(m->keyvalue_true);
            m->keyvalue_true = out;            
        } else {
            matte_heap_recycle(m->keyvalue_false);
            m->keyvalue_false = out;            
        }
        return;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
            matte_heap_recycle(*value);
            *value = out;
        } else {
            value = malloc(sizeof(matteValue_t));
            *value = out;
            matte_table_insert(m->keyvalues_object, &key, value);
        }
        return;
      }
    }

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
    out->keyvalues_number = matte_array_create(sizeof(matteValue_t));
    out->keyvalues_object = matte_table_create_hash_buffer(sizeof(matteValue_t));
    out->functionCaptures = matte_array_create(sizeof(CapturedReferrable_t));
    return out;
}

static void destroy_object(void * d) {
    matteObject_t * out = d;
    matte_table_destroy(out->keyvalues_string);
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
    out->toSweep = matte_table_create_hash_pointer();
    return out;
}



matteValue_t matte_heap_new_value(matteHeap_t * h) {
    matteValue_t out;
    out.heap = h;
    out.binID = 0;
    out.objectID = 0;
    return out;
}

void matte_value_into_empty(matteValue_t * v) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_EMPTY;
}

int matte_value_is_empty(matteValue_t v) {
    return v.binID == 0;
}


// Sets the type to a number with the given value
void matte_value_into_number(matteValue_t * v, double val) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_NUMBER;
    double * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], &v->objectID);
    *d = val;
}

// Sets the type to a number with the given value
void matte_value_into_boolean(matteValue_t * v, int val) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_BOOLEAN;
    int * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], &v->objectID);
    *d = val == 1;
}

void matte_value_into_string(matteValue_t * v, const matteString_t * str) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_STRING;
    matteString_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], &v->objectID);
    matte_string_set(d, str);
}

void matte_value_into_new_object_ref(matteValue_t * v) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->refs = 1;
}

void matte_value_into_new_object_literal_ref(matteValue_t * v, const matteArray_t * arr) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->refs = 1;


    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    
    for(i = 0; i < len; i+=2) {
        matteValue_t key   = matte_array_at(arr, matteValue_t, i);
        matteValue_t value = matte_array_at(arr, matteValue_t, i+1);
        
        object_put_prop(d, key, value);
    }

}


void matte_value_into_new_object_array_ref(matteValue_t * v, const matteArray_t * args) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
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

matteValue_t matte_value_function_get_named_referrable(matteValue_t v, const matteString_t * name) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return matte_heap_new_value(v.heap);
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    if (!m->functionstub) return matte_heap_new_value(v.heap);
    // claim captures immediately.
    uint32_t i;
    uint32_t len;



    // referrables come from a history of creation contexts.
    matteValue_t context = v;
    matteValue_t contextReferrable = m->origin_referrable;

    matteValue_t out = matte_heap_new_value(v.heap);

    while(context.binID) {
        matteObject_t * origin = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], context.objectID);


        len = matte_bytecode_stub_arg_count(origin->functionstub);
        for(i = 0; i < len; ++i) {
            if (matte_string_test_eq(matte_bytecode_stub_get_arg_name(origin->functionstub, i), name)) {
                matte_value_into_copy(&out, *matte_value_object_array_at_unsafe(contextReferrable, 1+i));
                return out;
            }
        }
        
        len = matte_bytecode_stub_local_count(origin->functionstub);
        for(i = 0; i < len; ++i) {
            if (matte_string_test_eq(matte_bytecode_stub_get_local_name(origin->functionstub, i), name)) {
                matte_value_into_copy(&out, *matte_value_object_array_at_unsafe(contextReferrable, 1+i+matte_bytecode_stub_arg_count(origin->functionstub)));
                return out;
            }
        }

        context = origin->origin;
        contextReferrable = origin->origin_referrable;
    }

    matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Could not find named referrable!"));
    return out;
}


void matte_value_into_new_function_ref(matteValue_t * v, matteBytecodeStub_t * stub) {
    matte_heap_recycle(*v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->heapID = v->objectID;
    d->refs = 1;
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
            if (matte_bytecode_stub_get_id(origin->functionstub) == capturesRaw[i].stubID) {
                CapturedReferrable_t ref;
                ref.referrableSrc = matte_heap_new_value(v->heap);
                matte_value_into_copy(&ref.referrableSrc, contextReferrable);
                ref.index = capturesRaw[i].referrable;

                matte_array_push(d->functionCaptures, ref);
                break;
            }
            context = origin->origin;
            contextReferrable = origin->origin_referrable;
        }

        if (!context.binID) {
            matte_vm_raise_error_string(v->heap->vm, MATTE_STR_CAST("Could not find captured variable!"));
            matteValue_t copy = matte_heap_new_value(v->heap);
            matte_array_push(d->functionCaptures, copy);
        }
    }
}


matteValue_t * matte_value_object_array_at_unsafe(matteValue_t v, uint32_t index) {
    matteObject_t * d = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    return &matte_array_at(d->keyvalues_number, matteValue_t, index);
}


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        return m->functionstub;
    }
    return NULL;
}

matteValue_t * matte_value_get_captured_value(matteValue_t v, uint32_t index) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
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
      case MATTE_VALUE_TYPE_OBJECT: 
        printf("  Heap Type: OBJECT\n");
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
    
    printf("\n");
    fflush(stdout);
}


void matte_value_set_object_userdata(matteValue_t v, void * userData) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        m->userdata = userData;
    }
}

void * matte_value_get_object_userdata(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        return m->userdata;
    }
    return NULL;
}



double matte_value_as_number(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: 
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert empty into a number."));
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
        matteValue_t * toNumber;
        if ((toNumber = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("toNumber")))) {
            return matte_value_as_number(matte_vm_call(v.heap->vm, *toNumber, matte_array_empty()));
        } else {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot convert object into a number. (no toNumber() property present.)"));
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
        matteValue_t toString = matte_value_object_access_string(v, MATTE_STR_CAST("toString"));
        if (toString.binID) {
            matteValue_t result = matte_vm_call(v.heap->vm, toString, matte_array_empty());
            matteString_t * out = matte_value_as_string(result);
            if (out) {
                out = matte_string_clone(out);
            } else {
                out = matte_string_create_from_c_str("");
            }
            matte_heap_recycle(result);
            matte_heap_recycle(toString);
            return out;
        } else {
            matte_heap_recycle(toString);
            return matte_string_create_from_c_str("object");
        }
      }
    }
    return matte_string_create();
}

int matte_value_as_boolean(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_NUMBER: {
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return *m != 0.0;
      }
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m;
      }
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return matte_string_test_eq(m, MATTE_STR_CAST("true")) ? 1 : 0;
      }
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        matteValue_t * toBoolean;
        if ((toBoolean = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("toBoolean")))) {
            return matte_value_as_number(matte_vm_call(v.heap->vm, *toBoolean, matte_array_empty()));
        } else {
            return 1;
        }
      }
    }
    return 0;
}

// returns whether the value is callable.
int matte_value_is_callable(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return 0;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    return m->functionstub != NULL;    
}

// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteValue_t v, matteValue_t key) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return matte_heap_new_value(v.heap);
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    matteValue_t * accessor;
    if ((accessor = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("accessor")))) {
        matteValue_t args[1] = {
            key
        };
        return matte_vm_call(v.heap->vm, *accessor, MATTE_ARRAY_CAST(args, matteValue_t, 1));
    } else {
        return object_lookup(m, key);
    }
}


matteValue_t matte_value_object_access_string(matteValue_t v, const matteString_t * key) {
    matteValue_t keyO = matte_heap_new_value(v.heap);
    matte_value_into_string(&keyO, key);
    matteValue_t result = matte_value_object_access(v, keyO);
    matte_heap_recycle(keyO);
    return result;
}

matteValue_t matte_value_object_access_index(matteValue_t v, uint32_t index) {
    matteValue_t keyO = matte_heap_new_value(v.heap);
    matte_value_into_number(&keyO, index);
    matteValue_t result = matte_value_object_access(v, keyO);
    matte_heap_recycle(keyO);
    return result;
}

matteValue_t matte_value_object_keys(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return matte_heap_new_value(v.heap);
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    matteArray_t * keys = matte_array_create(sizeof(matteValue_t));
    matteValue_t val = matte_heap_new_value(v.heap);
    // first number 
    uint32_t i;
    uint32_t len = matte_array_get_size(m->keyvalues_number);
    if (len) {
        for(i = 0; i < len; ++i) {
            val = matte_array_at(m->keyvalues_number, matteValue_t, i);
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
            val = *(matteValue_t*)matte_table_iter_get_value(iter);
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

    matte_value_into_new_object_array_ref(&val, keys);
    return val;
}

uint32_t matte_value_object_get_key_count(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        return 0;
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
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
    return total;    
}


void matte_value_object_remove_key(matteValue_t v, matteValue_t key) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return;
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue_t * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            matte_table_remove(m->keyvalues_string, str);
            matte_heap_recycle(*value);
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
        matte_heap_recycle(matte_array_at(m->keyvalues_number, matteValue_t, index));
        matte_array_remove(m->keyvalues_number, index);
        return;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);
        if (!d) return;
        if (*d) {
            matte_heap_recycle(m->keyvalue_true);
            m->keyvalue_true = matte_heap_new_value(v.heap);            
        } else {
            matte_heap_recycle(m->keyvalue_false);
            m->keyvalue_false = matte_heap_new_value(v.heap);               
        }
        return;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue_t * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
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
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2));
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
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2));
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
            matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2));
            matte_heap_recycle(r);
        }
        matte_table_iter_destroy(iter);
    }

    // true 
    if (m->keyvalue_true.binID) {
        matte_value_into_boolean(&args[0], 1);
        args[1] = m->keyvalue_true;
        matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2));        
        matte_heap_recycle(args[0]);
        matte_heap_recycle(r);
    }
    // false
    if (m->keyvalue_false.binID) {
        matte_value_into_boolean(&args[0], 1);
        args[1] = m->keyvalue_false;
        matteValue_t r = matte_vm_call(v.heap->vm, func, MATTE_ARRAY_CAST(args, matteValue_t, 2));        
        matte_heap_recycle(r);
        matte_heap_recycle(args[0]);
    }


}

void matte_value_object_set(matteValue_t v, matteValue_t key, matteValue_t value) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot set property on something that isnt an object."));
        return;
    }
    if (key.binID == 0) {
        matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot set property with an empty key"));
        return;
    }
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    matteValue_t * assigner;
    if ((assigner = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("assigner")))) {
        matteValue_t args[2] = {
            key, value
        };
        matteValue_t r = matte_vm_call(v.heap->vm, *assigner, MATTE_ARRAY_CAST(args, matteValue_t, 2));
        if (r.binID == MATTE_VALUE_TYPE_BOOLEAN && !matte_value_as_boolean(r)) {
        } else {
            object_put_prop(m, key, value); 
        }
        matte_heap_recycle(r);
    } else {
        object_put_prop(m, key, value);
    }
}


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy(matteValue_t * to, matteValue_t from) {
    if (to->binID)
        matte_heap_recycle(*to);

    switch(from.binID) {
      case MATTE_VALUE_TYPE_EMPTY:
        *to = from;
        return;

      case MATTE_VALUE_TYPE_NUMBER: 
        matte_value_into_number(to, *(double*)matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], from.objectID));
        return;
      case MATTE_VALUE_TYPE_BOOLEAN: 
        matte_value_into_boolean(to, *(int*)matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], from.objectID));
        return;
      case MATTE_VALUE_TYPE_STRING: 
        matte_value_into_string(to, matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], from.objectID));
        return;
      case MATTE_VALUE_TYPE_OBJECT: {
        matteObject_t * m = matte_bin_fetch(from.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], from.objectID);
        m->refs++;
        *to = from;
        return;
      }
    }
}




// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        if (m->refs) {
            m->refs--;
            // should guarantee no repeats
            if (m->refs == 0)
                matte_table_insert(v.heap->toSweep, m, (void*)0x1);                
        }
    } else if (v.binID) {
        matte_bin_recycle(v.heap->sortedHeaps[v.binID], v.objectID);
    }
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

        // might have been "saved" elsewhere.
        if (m->refs == 0) {
            matteValue_t * scriptFinalizer = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("finalizer"));

            if (scriptFinalizer) {
                matteValue_t object = matte_heap_new_value(h);
                object.binID = MATTE_VALUE_TYPE_OBJECT;
                object.objectID = m->heapID;

                // should be fine despite ref count being 0
                matte_vm_call(h->vm, *scriptFinalizer, MATTE_ARRAY_CAST(&object, matteValue_t, 1));
            }

            // might have been saved by finalizer
            if (m->refs == 0) {
                #ifdef MATTE_DEBUG__HEAP
                    printf("--RECYCLING OBJECT %d\n", m->heapID);
                #endif
                // watch out!!!! might have been re-entered in toSweep if finalizer 
                // logic saved then re-killed it... whoops...
                matte_table_remove(h->toSweep, m);

                // clean up object;
                m->functionstub = NULL;
                m->userdata = NULL;

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
                        matte_heap_recycle(matte_array_at(m->functionCaptures, matteValue_t, n));
                    }
                    matte_array_set_size(m->functionCaptures, 0);
                    matte_heap_recycle(m->origin);
                    m->origin.binID = 0;
                    matte_heap_recycle(m->origin_referrable);
                    m->origin_referrable.binID = 0;
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

                matte_bin_recycle(h->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], m->heapID);
            }
        }
    }
    matte_table_iter_destroy(iter);
    matte_array_destroy(sweep);
    h->gcLocked = 0;
}
