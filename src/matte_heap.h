#ifndef H_MATTE__HEAP__INCLUDED
#define H_MATTE__HEAP__INCLUDED


typedef struct matteString_t matteString_t;
typedef struct matteVM_t matteVM_t;
typedef struct matteBytecodeStub_t matteBytecodeStub_t;
typedef struct matteArray_t matteArray_t;
typedef struct matteVMStackFrame_t matteVMStackFrame_t;
#include <stdint.h>
typedef struct matteHeap_t matteHeap_t;


// Heap contains values
// Values can either hold light data (number / boolean / string)
// or point to objects.


matteHeap_t * matte_heap_create(matteVM_t *);

void matte_heap_destroy(matteHeap_t *);

typedef enum {
    MATTE_VALUE_TYPE_EMPTY,
    MATTE_VALUE_TYPE_BOOLEAN,
    MATTE_VALUE_TYPE_NUMBER,
    MATTE_VALUE_TYPE_STRING,
    MATTE_VALUE_TYPE_OBJECT,
    MATTE_VALUE_TYPE_TYPE
} matteValue_Type_t;


// a value object.
typedef struct  {
    matteHeap_t * heap;
    uint32_t binID;
    uint32_t objectID;    
} matteValue_t;

#ifdef MATTE_THIS_IS_JUST_FOR_VISUAL_REFERENCE
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

// Sets the type to an object that points to a new, empty object 
// with the given type.
void matte_value_into_new_object_ref_typed(matteValue_t *, matteValue_t type);



// Sets the type to a Type with the given typecode. Typecode values
// are only visible to the C / supervisor contexts.
void matte_value_into_new_type(matteValue_t *, matteValue_t opts);


void matte_value_into_new_object_literal_ref(matteValue_t *, const matteArray_t *);

// creates a new object with indexed keys/
void matte_value_into_new_object_array_ref(matteValue_t * v, const matteArray_t *);

void matte_value_into_new_function_ref(matteValue_t *, matteBytecodeStub_t *);

void matte_value_into_new_typed_function_ref(matteValue_t *, matteBytecodeStub_t * stub, const matteArray_t * args);


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy(matteValue_t *, matteValue_t from);

// returns a value to the heap.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_heap_recycle(matteValue_t);

// Marks a value as a root value, which means it, and any 
// value that traces back to this value, will not be garbage 
// collected.
//
// By default, only the internal referrable array is 
// set as root.
void matte_value_object_push_lock(matteValue_t);
void matte_value_object_pop_lock(matteValue_t);

#endif 
#include "matte_heap_alloc"






matteValue_t * matte_value_object_array_at_unsafe(matteValue_t v, uint32_t index);

// Under the assumption that value is a string, returns the internally
// kept string reference. This is significantly faster for the string 
// case, as a new string object does not need to be created.
const matteString_t * matte_value_string_get_string_unsafe(matteValue_t v);


// Given a function object, gets the in-scope referrable identified
// by the given name. If none can be found, an error is raised and 
// empty is returned.
matteValue_t matte_value_frame_get_named_referrable(matteVMStackFrame_t *, const matteString_t *);

// 
void matte_value_set_captured_value(matteValue_t v, uint32_t index, matteValue_t val);


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteValue_t);

// Gets all captured values by the function. If not a function, returns NULL
matteValue_t * matte_value_get_captured_value(matteValue_t, uint32_t index);

// Returns a subset of the value.
// For strings, this returns a new string object thats from the given index to the given index, inclusive.
// For arrays, this returns anew array object thats from the given index to the given end index, inclusive.
// If the bounds are invalid, empty is returned.
matteValue_t matte_value_subset(matteValue_t v, uint32_t from, uint32_t to);

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

int matte_value_as_boolean(matteValue_t);

// creates a new variable whose type is specified by "type".
// Returns empty on error.
// Assumes "type" is a valid type object.
matteValue_t matte_value_to_type(matteValue_t, matteValue_t type);

// returns whether the value is callable.
// 0 -> not callable 
// 1 -> callable 
// 2 -> callable + typestrict
int matte_value_is_callable(matteValue_t);

// assumes: 
//  - the first arg is a function 
//  - the function is typestrict
//  - the arguments given match the bytecode stubs number of arguments
int matte_value_object_function_pre_typecheck_unsafe(matteValue_t, const matteArray_t *);

// assumes the first arg is a function and that the function is typestrict
void matte_value_object_function_post_typecheck_unsafe(matteValue_t, matteValue_t);


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present. The accessor invoked depends 
// on which emulation method is used. isBracket denotes this.
matteValue_t matte_value_object_access(matteValue_t, matteValue_t key, int isBracket);

// convenience function. Same as matte_value_object_access except creates a temporaty 
// string object as a key. Dot (.) access is emulated.
matteValue_t matte_value_object_access_string(matteValue_t, const matteString_t *);

// Convenience function. Same as matte_value_object_access, except creates a 
// temporary number object Bracket ([]) access is emulated. 
matteValue_t matte_value_object_access_index(matteValue_t, uint32_t);



// If the value is an object, returns a new object with numbered 
// keys pointing to the keys of the original objects. If not an object,
// empty is returned
matteValue_t matte_value_object_keys(matteValue_t);

matteValue_t matte_value_object_values(matteValue_t);


// Returns the number of keys within the object.
// If not an object, 0 is returned.
uint32_t matte_value_object_get_key_count(matteValue_t);

// attempts to run a VM call for each key-value pair within the object.
// If object is not an object, no action is taken (and no error is raised.)
void matte_value_object_foreach(matteValue_t object, matteValue_t function);


// Attempts to set a key-value pair within the object.
// invokes assigner if present
const matteValue_t * matte_value_object_set(matteValue_t, matteValue_t key, matteValue_t value, int isBracket);

// Sets the operator set for this object, which describes how 
// the object should respond to different operators and interactions.
// Only operators referenced in the give operator set are 
// updated in the object. If the object already has operators 
// referenced by opObject, these operators are overwritten.
void matte_value_object_set_operator(matteValue_t v, matteValue_t opObject);

// Gets the operator for an object, assuming that the value is an object.
// If the object has no operator set, NULL is returned.
const matteValue_t * matte_value_object_get_operator_unsafe(matteValue_t);

// Removes a key from an object if it exists. If the value is 
// not an object or the key does not exist, no action is taken.
void matte_value_object_remove_key(matteValue_t, matteValue_t key);

// Convenience function that calls matte_value_object_remove_key 
// using a key value produced from a string
void matte_value_object_remove_key_string(matteValue_t, const matteString_t *);


// Given a value to an Object, creates a new string from the number 
// keys of the object. All number keys are searched in order.
// The function only succeeds correctly if ALL valid number keys are 
// number values. A string object is always created and returned.
// NOTE: the value is assumed to be an object. Hence the "unsafe".
matteValue_t matte_value_object_array_to_string_unsafe(matteValue_t);


// uniquely identifies the type (nonzero). Returns 0 if bad.
uint32_t matte_value_type_get_typecode(matteValue_t);



// Returns whether the value is of the type given by the typeobject typeobj.
int matte_value_isa(matteValue_t, matteValue_t typeobj);

// Gets a Type object of the given value.
matteValue_t matte_value_get_type(matteValue_t);

// Given a value type, returns the public name of it.
// Every type has a name.
const matteString_t * matte_value_type_name(matteValue_t);


// get built-in type object
const matteValue_t * matte_heap_get_empty_type(matteHeap_t *);
const matteValue_t * matte_heap_get_boolean_type(matteHeap_t *);
const matteValue_t * matte_heap_get_number_type(matteHeap_t *);
const matteValue_t * matte_heap_get_string_type(matteHeap_t *);
const matteValue_t * matte_heap_get_object_type(matteHeap_t *);
const matteValue_t * matte_heap_get_type_type(matteHeap_t *);
const matteValue_t * matte_heap_get_any_type(matteHeap_t *);

void matte_heap_garbage_collect(matteHeap_t *);

#endif
