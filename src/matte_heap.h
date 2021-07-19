#ifndef H_MATTE__HEAP__INCLUDED
#define H_MATTE__HEAP__INCLUDED


typedef struct matteString_t matteString_t;
typedef struct matteVM_t matteVM_t;
typedef struct matteBytecodeStub_t matteBytecodeStub_t;
typedef struct matteArray_t matteArray_t;
#include <stdint.h>


typedef struct matteHeap_t matteHeap_t;


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
typedef struct  {
    matteHeap_t * heap;
    uint32_t binID;
    uint32_t objectID;    
} matteValue_t;

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


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteValue_t);

// Gets all captured values by the function. If not a function, returns NULL
const matteArray_t * matte_value_get_captured_values(matteValue_t);


// if the value points to an object, sets custom data for the object.
void matte_value_set_object_userdata(matteValue_t, void *);

// if the value points to an object, returns the custom data for it.
// Else, returns NULL.
void * matte_value_get_object_userdata(matteValue_t);

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

// convenience function. Same as matte_value_object_access except creates a temporaty 
// string object as a key.
matteValue_t matte_value_object_access_string(matteValue_t, const matteString_t *);


// Attempts to set a key-value pair within the object.
// invokes assigner if present
void matte_value_object_set(matteValue_t, matteValue_t key, matteValue_t value);




// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy(matteValue_t *, matteValue_t from);




// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle(matteValue_t);



void matte_heap_garbage_collect(matteHeap_t *);

#endif
