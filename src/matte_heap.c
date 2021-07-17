
struct matteHeap_t {    
    // 1 -> number. Each pointer is just a double
    // 2 -> boolean 
    // 3 -> string 
    // 4 -> object
    matteBin_t * sortedHeaps[5];
    
    
    
    
    // objects with suspected ref count 0.
    // needs a double check before confirming.
    // matteObject_t *
    matteArray_t * toSweep;
    
    matteVM_t * vm;
}



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
        
    // called when about to be swept!
    int(finalizer*)(matteHeap_t *, matteValue_t, void *);     
    
    // custom data for the finalizer
    void * finalizerData;  

    // reference count
    uint32_t refs;
    
    // id within sorted heap.
    uint32_t heapID;
} matteObject_t;



static value_release(matteValue_t v) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        if (m->refs) {
            m->refs--;
            // should guarantee no repeats
            if (m->refs == 0)
                matte_array_push(v.heap->toSweep, m);                
        }
    } 
    
    if (v.binID) {
        matte_bin_recycle(v.heap->sortedHeaps[v.binID], v.objectID);
    }
    
    
}

static matteValue_t object_lookup(matteObject_t * m, matteValue_t key) {
    matteValue out = matte_heap_new_value(v.heap);
    switch(key.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return out;
      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * str = matte_bin_fetch(vey.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], key.objectID);
        matteValue * value = matte_table_find(m->keyvalues_string, str);
        if (value) {
            matte_value_into_copy(out, *value);
        }
        return out;
      }
      
      // array lookup
      case MATTE_VALUE_TYPE_NUMBER: {
        matteValue out = matte_heap_new_value(v.heap);
        double * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], key.objectID);
        uint32_t index = (uint32_t)*d;
        if (index < matte_array_get_size(m->keyvalues_number)) {
            matte_value_into_copy(out, matte_array_at(m->keyvalues_number, matteArray_t, index));
        }
        return out;
      }
      
      case MATTE_VALUE_TYPE_BOOLEAN: {
        matteValue out = matte_heap_new_value(v.heap);
        int * d = matte_bin_fetch(key.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], key.objectID);

        if (*d) {
            matte_value_into_copy(out, m->keyvalue_true);
        } else {
            matte_value_into_copy(out, m->keyvalue_false);
        }
        return out;
      }
      
      case MATTE_VALUE_TYPE_OBJECT: {           
        matteValue * value = matte_table_find(m->keyvalues_object, &key);
        if (value) {
            matte_value_into_copy(out, *value);
        }
        return out;
      }
    }
}

static void * create_double() {
    return malloc(sizeof(double));
}

static void destroy_double(void * d) {free(d};}

static void * create_boolean() {
    return malloc(sizeof(int));
}
static void destroy_boolean(void * d) {free(d};}

static void * create_string() {
    return matte_string_create();
}
static void destroy_string(void * d) {matte_string_destroy(d};}

static void * create_object() {
    matteObject_t * out = calloc(1, sizeof(matteObject_t));
    out->keyvalues_string = matte_table_create_hash_matte_string();
    out->keyvalues_number = matte_array_create(sizeof(matteValue_t));
    out->keyvalues_object = matte_table_create_hash_matte_buffer(sizeof(matteValue_t));
    return out;
}

static void destroy_object(void * d) {
    matteObject_t * out = d;
    matte_table_destroy(out->keyvalues_string);
    matte_table_destroy(out->keyvalues_object);
    matte_array_destroy(out->keyvalues_number);

    free(out);
}





matteHeap_t * matte_heap_create(matteVM_t *) {
    matteHeap_t * out = calloc(1, sizeof(matteHeap_t));
    out->sortedHeaps[MATTE_VALUE_TYPE_NUMBER] = matte_bin_create(create_double, destroy_double);
    out->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN] = matte_bin_create(create_boolean, destroy_boolean);
    out->sortedHeaps[MATTE_VALUE_TYPE_STRING] = matte_bin_create(create_string, destroy_string);
    out->sortedHeaps[MATTE_VALUE_TYPE_OBJECT] = matte_bin_create(create_object, destroy_object);
    return out;
}



matteValue_t matte_heap_new_value(matteHeap_t * h) {
    matteValue_t out;
    out.heap = h;
    out.binID = 0;
    out.objectID = 0;
    return out;
}

void matte_value_to_empty(matteValue_t * v) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_EMPTY;
}

int matte_value_is_empty(matteValue_t v) {
    return v.binID == 0;
}


// Sets the type to a number with the given value
void matte_value_into_number(matteValue_t * v, double val) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_NUMBER;
    double * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], &v->objectID);
    *d = val;
}

// Sets the type to a number with the given value
void matte_value_into_boolean(matteValue_t * v, int val) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_BOOLEAN;
    int * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], &v->objectID);
    *d = val == 1;
}

void matte_value_into_string(matteValue_t * v, const matteString_t * str) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_STRING;
    matteString_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], &v->objectID);
    matte_string_set(d, str);
}

void matte_value_into_new_object_ref(matteValue_t * v) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->refs = 1;
}

void matte_value_into_new_function_ref(matteValue_t *, matteBytecodeStub_t * stub) {
    value_release(v);
    v->binID = MATTE_VALUE_TYPE_OBJECT;
    matteObject_t * d = matte_bin_add(v->heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], &v->objectID);
    d->refs = 1;
    d->stub = stub;
}

void matte_value_set_finalizer(
    matteValue_t v, 
    int(finalizer*)(matteHeap_t *, matteValue_t, void *), 
    void * data) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        m->finalizer = finalizer;
        m->finalizerData = data;
    }
}

void matte_value_set_object_userdata(matteValue_t v, void * userData) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        m->userdata = userData;
    }
}

void * matte_value_get_object_userdata(matteValue_t) {
    if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        return m->userdata;
    }
    return NULL;
}



double matte_value_as_number(matteValue_t v) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_NUMBER:
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return *m;
      case MATTE_VALUE_TYPE_BOOLEAN:
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m;
      case MATTE_VALUE_TYPE_STRING:
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return strtod(matte_string_get_c_str(m));

      case MATTE_VALUE_TYPE_OBJECT:
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        matteValue_t * toNumber;
        if ((toNumber = matte_table_find(m->keyvalues, MATTE_STR_CAST("toNumber")))) {
            return matte_value_as_number(matte_vm_call(v.heap->vm, toNumber, matte_array_empty()));
        }
    }
    return 0;
}

matteString_t * matte_value_as_string(matteValue_t) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_NUMBER:
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return matte_string_create_from_c_str("%g", m);
      case MATTE_VALUE_TYPE_BOOLEAN:
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m ? matte_string_create_from_c_str("%s", "true") : matte_string_create_from_c_str("%s", "false");
      case MATTE_VALUE_TYPE_STRING:
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return matte_string_clone(m);

      case MATTE_VALUE_TYPE_OBJECT:
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        matteValue_t * toString;
        if ((toString = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("toString")))) {
            return matte_value_as_string(matte_vm_call(v.heap->vm, toString, matte_array_empty()));
        }
    }
    return matte_string_create();
}

int matte_value_as_boolean(matteValue_t) {
    switch(v.binID) {
      case MATTE_VALUE_TYPE_EMPTY: return 0;
      case MATTE_VALUE_TYPE_NUMBER:
        double * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_NUMBER], v.objectID);
        return m* == 0.0;
      case MATTE_VALUE_TYPE_BOOLEAN:
        int * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_BOOLEAN], v.objectID);
        return *m;
      case MATTE_VALUE_TYPE_STRING:
        matteString_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_STRING], v.objectID);
        return matte_string_test_eq(m, MATTE_STR_CAST("true")) ? 1 : 0;

      case MATTE_VALUE_TYPE_OBJECT:
        matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
        matteValue_t * toBoolean;
        if ((toBoolean = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("toNumber")))) {
            return matte_value_as_number(matte_vm_call(v.heap->vm, toBoolean, matte_array_empty()));
        }
    }
    return 0;
}

// returns whether the value is callable.
int matte_value_is_callable(matteValue_t v) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return 0;
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    return m->stub != NULL;    
}

// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteValue_t v, matteValue_t key) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return matte_value_new_value(v.heap);
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    matteValue_t * accessor;
    if ((accessor = matte_table_find(m->keyvalues_string, MATTE_STR_CAST("accessor")))) {
        matteValue_t args[2] = {
            v, key
        };
        return matte_vm_call(v.heap->vm, accessor, MATTE_ARR_CAST(args, matteValue_t, 2));
    } else {
        return object_lookup(m, key);
    }
}

matteValue_t matte_value_object_set(matteValue_t v, matteValue_t key, matteValue_t value) {
    if (v.binID != MATTE_VALUE_TYPE_OBJECT) return matte_value_new_value(v.heap);
    matteObject_t * m = matte_bin_fetch(v.heap->sortedHeaps[MATTE_VALUE_TYPE_OBJECT], v.objectID);
    matteValue_t * assigner;
    if ((accessor = matte_table_find(m->keyvalues, MATTE_STR_CAST("assigner")))) {
        matteValue_t args[3] = {
            v, key, value
        };
        matte_vm_call(v.heap->vm, assigner, MATTE_ARR_CAST(args, matteValue_t, 3));
    } else {
    
        matteValue_t v = object_lookup(m, key);
        if (v.binID == 0) {
            matte_vm_raise_error_string(v.heap->vm, MATTE_STR_CAST("Cannot set to an empty value"));
            return v;
        } else {

        }
    }

}

// Sets the data for this value. This invokes the assigner if present.
void matte_value_assign(matteValue_t, matteValue_t value);



// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy(matteValue_t, matteValue_t from);




// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle(matteValue_t);



void matte_heap_garbage_collect(matteHeap_t *);
