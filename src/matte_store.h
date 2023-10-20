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
#ifndef H_MATTE__STORE__INCLUDED
#define H_MATTE__STORE__INCLUDED


#include "matte_opcode.h"
typedef struct matteString_t matteString_t;
typedef struct matteVM_t matteVM_t;
typedef struct matteBytecodeStub_t matteBytecodeStub_t;
typedef struct matteArray_t matteArray_t;
typedef struct matteVMStackFrame_t matteVMStackFrame_t;
#include <stdint.h>
typedef struct matteStore_t matteStore_t;


// Store contains values
// Values can either hold light data (number / boolean / string)
// or point to objects.


matteStore_t * matte_store_create(matteVM_t *);

void matte_store_destroy(matteStore_t *);

typedef enum {
    MATTE_VALUE_TYPE_EMPTY,
    MATTE_VALUE_TYPE_BOOLEAN,
    MATTE_VALUE_TYPE_NUMBER,
    MATTE_VALUE_TYPE_STRING,
    MATTE_VALUE_TYPE_OBJECT,
    MATTE_VALUE_TYPE_TYPE
} matteValue_Type_t;

#define matte_value_type(__V__) ((__V__).binID)

#define matte_value_is_function(__V__) ((__V__).binID == MATTE_VALUE_TYPE_OBJECT && ((__V__).value.id/2)*2 == (__V__.value.id))


// a value object.
typedef struct  {
    uint32_t binID;
    union {
        int boolean;
        double number;
        uint32_t id; // for string, type, and object
    } value;
} matteValue_t;

#ifdef MATTE_THIS_IS_JUST_FOR_VISUAL_REFERENCE
// Creates a new empty value
matteValue_t matte_store_new_value(matteStore_t *);

// Sets the type to empty
void matte_value_into_empty(matteStore_t *, matteValue_t *);

// Sets the type to a number with the given value
void matte_value_into_number(matteStore_t *, matteValue_t *, double);

// Sets the type to a number with the given value
void matte_value_into_boolean(matteStore_t *, matteValue_t *, int);

// Sets the type to a number with the given value
void matte_value_into_string(matteStore_t *, matteValue_t *, const matteString_t *);

// Sets the type to an object that points to a new, empty object.
void matte_value_into_new_object_ref(matteStore_t *, matteValue_t *);

// Sets the type to an object that points to a new, empty object 
// with the given type.
void matte_value_into_new_object_ref_typed(matteStore_t *, matteValue_t *);



// Sets the type to a Type with the given typecode. Typecode values
// are only visible to the C / supervisor contexts.
void matte_value_into_new_type(matteStore_t *, matteValue_t *, matteValue_t name, matteValue_t inherits);


void matte_value_into_new_object_literal_ref(matteStore_t *, matteValue_t *, const matteArray_t *);

// creates a new object with indexed keys/
void matte_value_into_new_object_array_ref(matteStore_t *, matteValue_t * v, const matteArray_t *);

void matte_value_into_new_function_ref(matteStore_t *, matteValue_t *, matteBytecodeStub_t *);


// Like regular functions exept they are ignored in garbage collection.
void matte_value_into_new_external_function_ref(matteStore_t *, matteValue_t *, matteBytecodeStub_t *);

void matte_value_into_new_typed_function_ref(matteStore_t *, matteValue_t *, matteBytecodeStub_t * stub, const matteArray_t * args);


// if number/string/boolean: copy
// else: point to same source object
void matte_value_into_copy(matteStore_t *, matteValue_t *, matteValue_t from);

// returns a value to the store.
// if the value was pointing to an object / function,
// that object's ref count is decremented.
void matte_store_recycle(matteStore_t *, matteValue_t);

// Marks a value as a root value, which means it, and any 
// value that traces back to this value, will not be garbage 
// collected.
//
// By default, only the internal referrable array is 
// set as root.
void matte_value_object_push_lock(matteStore_t *, matteValue_t);
void matte_value_object_pop_lock(matteStore_t *, matteValue_t);

// clones an activated function into a new reference
void matte_value_into_cloned_function_ref(matteStore_t *, matteValue_t * v, matteValue_t source);


#endif 
#include "matte_store_alloc"






// Activates the closure references for the function, preparing it for calling.
// Referrables are the shadow references held by the function from local 
// objects and values.
// NOTE: values are NOT copied, but are transfered and then linked 
// to the function. The callers references are transfered (moved)
void matte_value_object_function_activate_closure(matteStore_t *, matteValue_t v, matteArray_t * );


// return whether the function was activated
int matte_value_object_function_was_activated(matteStore_t *, matteValue_t v);





void matte_value_object_function_set_closure_value(matteStore_t *, matteValue_t v, uint32_t, matteValue_t newVal);

matteValue_t * matte_value_object_function_get_closure_value(matteStore_t *, matteValue_t v, uint32_t);


void matte_value_object_set_is_interface(matteStore_t *, matteValue_t v, int enabled);


matteValue_t * matte_value_object_array_at_unsafe(matteStore_t *, matteValue_t v, uint32_t index);

// Under the assumption that value is a string, returns the internally
// kept string reference. This is significantly faster for the string 
// case, as a new string object does not need to be created.
const matteString_t * matte_value_string_get_string_unsafe(matteStore_t *, matteValue_t v);


// Given a function object, gets the in-scope referrable identified
// by the given name. If none can be found, an error is raised and 
// empty is returned.
matteValue_t matte_value_frame_get_named_referrable(matteStore_t * store, matteVMStackFrame_t *, matteValue_t);

// 
void matte_value_set_captured_value(matteStore_t *, matteValue_t v, uint32_t index, matteValue_t val);


matteBytecodeStub_t * matte_value_get_bytecode_stub(matteStore_t *, matteValue_t);

// Gets all captured values by the function. If not a function, returns NULL
matteValue_t * matte_value_get_captured_value(matteStore_t *, matteValue_t, uint32_t index);

// Returns a subset of the value.
// For strings, this returns a new string object thats from the given index to the given index, inclusive.
// For arrays, this returns anew array object thats from the given index to the given end index, inclusive.
// If the bounds are invalid, empty is returned.
matteValue_t matte_value_subset(matteStore_t *, matteValue_t v, uint32_t from, uint32_t to);

// if the value points to an object, sets custom data for the object.
void matte_value_object_set_userdata(matteStore_t *, matteValue_t, void *);

// if the value points to an object, returns the custom data for it.
// Else, returns NULL.
void * matte_value_object_get_userdata(matteStore_t *, matteValue_t);

// Registers a function to call with the object's userdata
// and given data after the object has been cleaned up by 
// the VM. This is useful for native implementations 
// that use object wrappers which require explicit 
// cleanup usually by user code. This keeps code 
// simpler within the Matte context since the native 
// impllementation can avoid requiring this explicit 
// cleanup since it can be automated.
void matte_value_object_set_native_finalizer(matteStore_t *, matteValue_t, void (*)(void * objectUserdata, void * functionUserdata), void * functionUserData);

// returns whether the matte value is empty
int matte_value_is_empty(matteStore_t *, matteValue_t);

double matte_value_as_number(matteStore_t *, matteValue_t);

// user must free string
matteValue_t matte_value_as_string(matteStore_t *, matteValue_t);

int matte_value_as_boolean(matteStore_t *, matteValue_t);

// creates a new variable whose type is specified by "type".
// Returns empty on error.
// Assumes "type" is a valid type object.
matteValue_t matte_value_to_type(matteStore_t *, matteValue_t, matteValue_t type);

// returns whether the value is callable.
// 0 -> not callable 
// 1 -> callable 
// 2 -> callable + typestrict
int matte_value_is_callable(matteStore_t *, matteValue_t);

// assumes: 
//  - the first arg is a function 
//  - the function is typestrict
//  - the arguments given match the bytecode stubs number of arguments
int matte_value_object_function_pre_typecheck_unsafe(matteStore_t *, matteValue_t, const matteArray_t *);

// assumes the first arg is a function and that the function is typestrict
void matte_value_object_function_post_typecheck_unsafe(matteStore_t *, matteValue_t, matteValue_t);


// If the value points to an object, returns the value associated with the 
// key. This will invoke the accessor if present. The accessor invoked depends 
// on which emulation method is used. isBracket denotes this.
matteValue_t matte_value_object_access(matteStore_t *, matteValue_t, matteValue_t key, int isBracket);


// When available, returns a
matteValue_t * matte_value_object_access_direct(matteStore_t *, matteValue_t, matteValue_t key, int isBracket);


// convenience function. Same as matte_value_object_access except creates a temporaty 
// string object as a key. Dot (.) access is emulated.
matteValue_t matte_value_object_access_string(matteStore_t *, matteValue_t, const matteString_t *);

// Convenience function. Same as matte_value_object_access, except creates a 
// temporary number object Bracket ([]) access is emulated. 
matteValue_t matte_value_object_access_index(matteStore_t *, matteValue_t, uint32_t);

// Convenience function. Removes all number'keyed methods from the 
// object table, resetting the key count back to zero for number keys.
// the value is assumed to be a table object.
void matte_value_object_clear_number_keys_unsafe(matteStore_t *, matteValue_t);

// If the value is an object, returns a new object with numbered 
// keys pointing to the keys of the original objects. If not an object,
// empty is returned
matteValue_t matte_value_object_keys(matteStore_t *, matteValue_t);

matteValue_t matte_value_object_values(matteStore_t *, matteValue_t);


// Returns the number of keys within the object.
// If not an object, 0 is returned.
uint32_t matte_value_object_get_key_count(matteStore_t *, matteValue_t);

// Returns the number of number keys within the object, ignoring keys of other types.
uint32_t matte_value_object_get_number_key_count(matteStore_t *, matteValue_t);

// Inserts a numbered key into the object. If there are any number-keyed values 
// whose keys are above the given index, those key-values are pushed up.
void matte_value_object_insert(matteStore_t *, matteValue_t, uint32_t key, matteValue_t val);

// Efficient case of matte_value_object_insert with key == object keycount
void matte_value_object_push(
    matteStore_t * store, 
    matteValue_t v, 
    matteValue_t val
);


// Sorts the number key contents of the 
void matte_value_object_sort_unsafe(matteStore_t *, matteValue_t, matteValue_t less);
// attempts to run a VM call for each key-value pair within the object.
// If object is not an object, no action is taken (and no error is raised.)
void matte_value_object_foreach(matteStore_t *, matteValue_t object, matteValue_t function);


// Attempts to set a key-value pair within the object.
// invokes assigner if present
matteValue_t matte_value_object_set(matteStore_t *, matteValue_t, matteValue_t key, matteValue_t value, int isBracket);


// Same as matte_value_object_set() but is handy for string insertion
matteValue_t matte_value_object_set_key_string(matteStore_t *, matteValue_t, const matteString_t * key, matteValue_t value);



// Performs a matte_value_object_set() with dot access for each 
// key-value pair in srcTable. If srcTable is not a table, an error is raised.
//
void matte_value_object_set_table(matteStore_t *, matteValue_t, matteValue_t srcTable);



// Given that the value is an object, DIRECTLY sets a numbered-index value, bypassing 
// the attribute overrides and any safety checks. Only use if you know what youre doing.
void matte_value_object_set_index_unsafe(matteStore_t *, matteValue_t, uint32_t i, matteValue_t value);


// Sets the attributes set for this object, which describes how 
// the object should respond to different operators and events.
// This existing attributes object is replaced with this new object.
// If this new object is the same as the original object, no change is made.
void matte_value_object_set_attributes(matteStore_t *, matteValue_t v, matteValue_t opObject);

// Gets the attributes set for an object, assuming that the value is an object.
// If the object has no operator set, NULL is returned.
const matteValue_t * matte_value_object_get_attributes_unsafe(matteStore_t *, matteValue_t);

// Removes a key from an object if it exists. If the value is 
// not an object or the key does not exist, no action is taken.
void matte_value_object_remove_key(matteStore_t *, matteValue_t, matteValue_t key);

// Convenience function that calls matte_value_object_remove_key 
// using a key value produced from a string
void matte_value_object_remove_key_string(matteStore_t *, matteValue_t, const matteString_t *);


// uniquely identifies the type (nonzero). Returns 0 if bad.
uint32_t matte_value_type_get_typecode(matteValue_t);


// Returns whether the value is of the type given by the typeobject typeobj.
int matte_value_isa(matteStore_t *, matteValue_t, matteValue_t typeobj);

// Gets a Type object of the given value.
matteValue_t matte_value_get_type(matteStore_t *, matteValue_t);

matteValue_t matte_value_query(matteStore_t * store, matteValue_t * v, matteQuery_t query);

// Given a value type, returns the public name of it.
// Every type has a name.
matteValue_t matte_value_type_name(matteStore_t *, matteValue_t);

void matte_store_push_lock_gc(matteStore_t *);
void matte_store_pop_lock_gc(matteStore_t *);

// get built-in type object
const matteValue_t * matte_store_get_empty_type(matteStore_t *);
const matteValue_t * matte_store_get_boolean_type(matteStore_t *);
const matteValue_t * matte_store_get_number_type(matteStore_t *);
const matteValue_t * matte_store_get_string_type(matteStore_t *);
const matteValue_t * matte_store_get_object_type(matteStore_t *);
const matteValue_t * matte_store_get_function_type(matteStore_t *);
const matteValue_t * matte_store_get_type_type(matteStore_t *);
const matteValue_t * matte_store_get_any_type(matteStore_t *);

void matte_store_garbage_collect(matteStore_t *);

#endif
