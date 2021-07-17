#ifndef H_MATTE__HEAP__INCLUDED
#define H_MATTE__HEAP__INCLUDED


typedef struct matteHeap_t matteHeap_t;
typedef struct matteValue_t matteValue_t;




// Heap contains values
// Values can either hold light data (number / boolean / string)
// or point to objects.


matteHeap_t * matte_heap_create(matteVM_t *);

typedef enum {
    MATTE_VALUE_TYPE_EMPTY,
    MATTE_VALUE_TYPE_NUMBER,
    MATTE_VALUE_TYPE_BOOLEAN,
    MATTE_VALUE_TYPE_STRING,
    MATTE_VALUE_TYPE_OBJECT,
} matteValue_Type_t;

// a value object.
// The object is private;
struct matteValue_t {
    matteHeap_t * heap;
    uint32_t binID;
    uint32_t objectID;    
};

// Creates a new empty value
matteValue_t matte_heap_new_value(matteHeap_t *);


// Sets the type to empty
void matte_value_into_empty(matteValue_t *);

// Sets the type to a number with the given value
void matte_value_into_number(matteValue_t *, double);

// Sets the type to a number with the given value
void matte_value_into_boolean(matteValue_t *, int);

// Sets the type to a number with the given value
void matte_value_into_string(matteValue_t *, const matteString_t *);

// Sets the type to an object that points to a new, empty object.
void matte_value_into_new_object_ref(matteValue_t *);

// 
void matte_value_into_new_function_ref(matteValue_t *, matteBytecodeStub_t *);

// if the value points to an object, can set a user function finalizer
void matte_value_set_finalizer(matteValue_t, int(*)(matteHeap_t *, matteValue_t, void *), void *);

// if the value points to an object, sets custom data for the object.
void matte_value_set_object_userdata(matteValue_t, void *);

// if the value points to an object, returns the custom data for it.
// Else, returns NULL.
void * matte_value_get_object_userdata(matteValue_t, void *);

// returns whether the matte value is empty
int matte_value_is_empty(matteValue_t);

double matte_value_as_number(matteValue_t);

// user must free string
matteString_t * matte_value_as_string(matteValue_t);

int matte_value_as_boolean(const matteValue_t);

// returns whether the value is callable.
int matte_value_is_callable(const matteValue_t);

// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present.
matteValue_t matte_value_object_access(matteValue_t, matteValue_t key);

// Attempts to set a key-value pair within the object.
// incokes assigner if presetnt
matteValue_t matte_value_object_set(matteValue_t, matteValue_t key, matteValue_t value);

matteValue_t matte_value_object_to_number(matteValue_t);
matteValue_t matte_value_object_to_string(matteValue_t);
matteValue_t matte_value_object_to_number(matteValue_t);

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

#endif
