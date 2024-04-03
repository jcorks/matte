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



/// The type of a value. Every value in Matte has an associated 
/// type that is static and maintained for that value.
typedef enum {
    /// The empty type is used for the empty value. All empty values are of the empty type.
    MATTE_VALUE_TYPE_EMPTY,
    /// Booleans can hold either true or false.
    MATTE_VALUE_TYPE_BOOLEAN,
    /// Numbers represent all number types. This is guaranteed to be at least 
    /// the size of a double. In this implementation it is equal to a double.
    MATTE_VALUE_TYPE_NUMBER,
    /// A string of characters.
    MATTE_VALUE_TYPE_STRING,
    /// An associative table with enhanced features for array operation as well as functions.
    /// A potentially confusing aspect is that on the implementation level, functions and objects 
    /// are treated are called "Objects", but in the Matte level they are distinct types.
    /// This collective mentioning mostly refers to the matteValue_Type_t, as in most other cases 
    /// Objects and Functions are treated differently, such as matte_store_ functions.
    MATTE_VALUE_TYPE_OBJECT,
    /// A value representing a type.
    MATTE_VALUE_TYPE_TYPE
} matteValue_Type_t;


typedef enum {
    MATTE_VALUE_EXTENDED_FLAG_NONE,
    // When the id == 0 and this flag is active, 
    // the id points to the empty function.
    MATTE_VALUE_EXTENDED_FLAG_EMPTY_FUNCTION = 1,
} matteValue_ExtendedFlag_t;


/// Every operation that works with Matte values on the C level 
/// refers to those values as matteValue_t instances.
///
/// Values that are of the type BOOLEAN, STRING, NUMBER, and TYPE are safe 
/// to copy matteValue_t by value in C and otherwise do not. Otherwise, the value must be copied 
/// (matte_value_into_copy) from existing source or used as-is.
///
typedef struct  {
    /// The type of the value.
    uint32_t binID;
    
    /// Collection of data that holds the value's data.
    union {
        /// For booleans, this is either 0 or 1. Others are not used.
        int boolean;
        /// For numbers, this is the actual numeric value. Others are not used.
        double number;
        /// For string, type, and Object, this is an ID uniquely identifying the value among its type.
        uint32_t id; 
        /// For when auxiliary data is needed in addition to an id 
        struct {
            uint32_t id;
            // For devs: MAKE SURE that if you modify the VM to use auxiliary IDs that the 
            // created objects clean their aux ID on creation! The only ones that are cleaned right 
            // now are Functions, as they are used for dynamic binding.
            uint32_t idAux;
        } extended;
    } value;
} matteValue_t;

/// macro for quickly determining if a funciton is the empty function.
#define matte_value_is_empty_function(__V__) ((__V__).binID == MATTE_VALUE_TYPE_OBJECT && (__V__).value.id == 0)

/// macro for getting the type of a matteValue_t
#define matte_value_type(__V__) ((__V__).binID)

/// (macro) Function for returning whether the value is a function.
#define matte_value_is_function(__V__) ((__V__).binID == MATTE_VALUE_TYPE_OBJECT && ((__V__).value.id/2)*2 == (__V__.value.id))




/// Matte Store 
///
/// The store is the major component of the VM that acts as the storage point for 
/// all working Matte values. In addition, it provides the bridge between the Matte 
/// context and the VM context for working with values explicitly. Many of the same operations 
/// in Matte source can be performed via the functions provided here.
///
/// The functions here, unless they return a pointer to a valueType, return 
/// matteValue_t values that must be recycled when done using matte_store_recycle().
/// This ensures proper cleanup of values. Please take note of when it is appropriate 
/// to recycle and not, as over-recycling can mark items for cleanup earlier than 
/// intended.
///
/// Note also that many of these functions will always return a value matteValue_t 
/// when relevant, but may through a VM error.
///
typedef struct matteStore_t matteStore_t;


/// Creates a new store based on the given VM instance.
/// This is normally not needed, as the matte_t instance will 
/// create one on your behalf. See matte.h.
///
matteStore_t * matte_store_create(matteVM_t *);

/// Destroys the store.
///
void matte_store_destroy(matteStore_t *);


/// Returns the special ID for the empty function.
matteValue_t matte_store_empty_function(matteStore_t *);




/// For debugging and other purposes, "real" references to these functions 
/// are held elsewere and wrapped. In practice, this macro can be ignored and the 
/// rest of the header used as documentation.

#ifdef MATTE_THIS_IS_JUST_FOR_VISUAL_REFERENCE
/// Creates a new empty value.
/// Working with new non-empty values in Matte first must come 
/// from a pre-existing value. Thus, new values are the first step 
/// to creating other values.
matteValue_t matte_store_new_value(matteStore_t *);


/// Changes the value into empty. 
///
void matte_value_into_empty(matteStore_t *, matteValue_t *);

/// Changes the value into a number with the given numeric value
void matte_value_into_number(matteStore_t *, matteValue_t *, double);

/// Changes the value into a boolean with the given state.
void matte_value_into_boolean(matteStore_t *, matteValue_t *, int);

/// Changes the value into a string with the given state.
void matte_value_into_string(matteStore_t *, matteValue_t *, const matteString_t *);

/// Changes the value into a new Object with a new lifetime.
/// Note that obejcts are garbage collected, so they should be used 
/// or associated with other alive Objects so that it is not 
/// garbage collected early.
void matte_value_into_new_object_ref(matteStore_t *, matteValue_t *);

/// Changes the value into a new Object with the given type. These types are not 
/// the built-in types, but user-defined types (which are always inherited from Object).
/// Note that obejcts are garbage collected, so they should be used 
/// or associated with other alive Objects so that it is not 
/// garbage collected early.
void matte_value_into_new_object_ref_typed(matteStore_t *, matteValue_t *, matteValue_t typeval);


/// Convenience function that works like matte_value_into_new_object_ref(), but 
/// uses an array to load the Object with values. The array is expected to be 
/// packed with matteValue_t values in pairs of key and value laid out in the array 
/// linearly.
void matte_value_into_new_object_literal_ref(matteStore_t *, matteValue_t *, const matteArray_t * keyvalues);

/// Convenience function that works like matte_value_into_new_object_ref(), but 
/// uses an array to load the Object with values. This creates 
/// an Object with number-indexed keys starting at 0, each with a value correcponsing 
/// to the values array input. Each member of values should be a matteValue_t.
void matte_value_into_new_object_array_ref(matteStore_t *, matteValue_t * v, const matteArray_t * values);


/// Changes the value into a function with the given compiled function 
/// information (bytecode stub).
void matte_value_into_new_function_ref(matteStore_t *, matteValue_t *, matteBytecodeStub_t *);


/// Equivalent to matte_value_into_new_function_ref(), except the function is
/// is marked as external. External functions are never cleaned and remain 
/// persistent until the store is destroyed.
void matte_value_into_new_external_function_ref(matteStore_t *, matteValue_t *, matteBytecodeStub_t *);

/// Changes the value into a new typestrict function.
/// This means the function arguments will follow the types specified 
/// in the given argTypes array, which should be of matteValue_t which 
/// point to Type values.
void matte_value_into_new_typed_function_ref(
    matteStore_t *, 
    matteValue_t *, 
    matteBytecodeStub_t * stub, 
    const matteArray_t * argTypes
);




/// Creates a new base type. Note that this creates an entirely unique Type.
/// If you wish to clone a value, simply copy a matteValue_t referring to a type.
matteValue_t matte_value_create_type(matteStore_t *, matteValue_t name, matteValue_t inherits, matteValue_t layout);


/// Safely changes the value into a copy of an existing value.
///
void matte_value_into_copy(matteStore_t *, matteValue_t *, matteValue_t from);

/// Safely tells the store that a value's use for a context is done.
/// For each time a value is created or copied via a function, 
/// it must be recycled unless it is a NUMBER, BOOLEAN, or TYPE
void matte_store_recycle(matteStore_t *, matteValue_t);

/// Marks a value as a root value, which means it, and any 
/// value that traces back to this value, will not be garbage 
/// collected.
/// This is normally called on your behalf for intermediate operations, 
/// such as calling a function, but can be useful for long-term 
/// objects that arent always explicit, such as internal values.
void matte_value_object_push_lock(matteStore_t *, matteValue_t);

/// Pops a lock on an object. When an value has its lock popped 
/// for as many times as it was pushed, that value will no longer 
/// be a root and will be available for garbage collection.
void matte_value_object_pop_lock(matteStore_t *, matteValue_t);

/// clones an activated function into a new reference. This copies the activated functions 
/// referred-to captured variables and arguments (referrables). This is normally not 
/// needed by user code, but is part of the normal function calling process internally.
///
void matte_value_into_cloned_function_ref(matteStore_t *, matteValue_t * v, matteValue_t source);


#endif 
#include "matte_store_alloc"








/// Sets whether an Object value is an interface. Interfaces only have 
/// string members, which are either functions or getter/setter objects.
/// As such, interface objects will also throw errors if:
/// - writing to an attribute that doesnt have a setter 
/// - reading an attribute that doesnt have a getter
/// - writing to a function attribute 
/// - accessing an interface with a non-string key.
///
/// An interface can also have a custom dynamic binding object. This is passed to the 
/// object's members (including setters + getters) in the interface's stead, allowing 
/// for a private datastore for the interface.
void matte_value_object_set_is_interface(matteStore_t *, matteValue_t v, int enabled, matteValue_t dynamicInterface);


/// Gets whether an object is an interface.
int matte_value_object_get_is_interface_unsafe(matteStore_t *, matteValue_t v);

/// Gets an objects private interface binding
/// If there is no private binding, empty is returned.
matteValue_t matte_value_object_get_interface_private_binding_unsafe(
    matteStore_t * store,
    matteValue_t v
);


/// Gets a reference (no copying) of a value within an object's 
/// numbered keyed pairs with no checking of the bounds or whether the 
/// value is an object. This is intended for fast access and should only 
/// be used when sure of inputs.
matteValue_t * matte_value_object_array_at_unsafe(matteStore_t *, matteValue_t v, uint32_t index);

/// Under the assumption that value is a string, returns the internally
/// kept string reference. This is significantly faster for the string 
/// case, as a new string object does not need to be created.
const matteString_t * matte_value_string_get_string_unsafe(matteStore_t *, matteValue_t v);

/// Sets the size of the array, internally resizing if needed.
/// This works without checking whether the object is an Object.
void matte_value_object_array_set_size_unsafe(matteStore_t *, matteValue_t v, uint32_t index);

/// Given a function object, gets the in-scope referrable identified
/// by the given name. If none can be found, an error is raised and 
/// empty is returned.
matteValue_t matte_value_frame_get_named_referrable(matteStore_t * store, matteVMStackFrame_t *, matteValue_t);


/// Gets the bytecode stub referred to by the function.
/// If not a function or is otherwise inaccessible, NULL is returned and 
/// no error is thrown.
///
matteBytecodeStub_t * matte_value_get_bytecode_stub(matteStore_t *, matteValue_t);


/// Returns a subset of the value.
/// For strings, this returns a new string object thats from the given index to the given index, inclusive.
/// For arrays, this returns anew array object thats from the given index to the given end index, inclusive.
/// If the bounds are invalid, empty is returned.
matteValue_t matte_value_subset(matteStore_t *, matteValue_t v, uint32_t from, uint32_t to);

/// If the value points to an object, sets custom data for the object.
/// This is particularly useful for custom C-interfacing Objects.
void matte_value_object_set_userdata(matteStore_t *, matteValue_t, void *);

/// If the value points to an object, returns the custom data for it.
/// Else, returns NULL. NULL is also returned if there was no data set.
void * matte_value_object_get_userdata(matteStore_t *, matteValue_t);

/// Registers a function to call with the object's userdata
/// and given data after the object has been cleaned up by 
/// the VM. This is useful for native implementations 
/// that use object wrappers which require explicit 
/// cleanup usually by user code. This keeps code 
/// simpler within the Matte context since the native 
/// implementation can avoid requiring this explicit 
/// cleanup since it can be automated.
void matte_value_object_set_native_finalizer(
    matteStore_t *, 
    matteValue_t, 
    void (*)(void * objectUserdata, void * functionUserdata), 
    void * functionUserData
);

/// Returns whether the matte value is empty.
int matte_value_is_empty(matteStore_t *, matteValue_t);

/// Returns a best-try conversion into a double.
double matte_value_as_number(matteStore_t *, matteValue_t);

/// Returns a best-try conversion into a string.
matteValue_t matte_value_as_string(matteStore_t *, matteValue_t);

/// Returns a best-try conversion into a boolean.
int matte_value_as_boolean(matteStore_t *, matteValue_t);

/// Returns a conversion of v whose type is specified by "type".
/// Returns empty on error.
/// Assumes "type" is a valid type value.
matteValue_t matte_value_to_type(matteStore_t *, matteValue_t v, matteValue_t type);

/// Returns whether the value is callable.
/// 0 -> not callable 
/// 1 -> callable 
/// 2 -> callable + typestrict
int matte_value_is_callable(matteStore_t *, matteValue_t);


/// If the value points to an object, returns the value associated with the 
/// key. This will invoke the accessor if present. The accessor invoked depends 
/// on which emulation method is used. isBracket denotes this.
matteValue_t matte_value_object_access(matteStore_t *, matteValue_t, matteValue_t key, int isBracket);


/// When available, returns an Object-owned version of the value directly.
/// If none is available, NULL is returned.
matteValue_t * matte_value_object_access_direct(matteStore_t *, matteValue_t, matteValue_t key, int isBracket);


/// Convenience function. Same as matte_value_object_access except creates a temporary 
/// string object as a key. Dot (.) access is emulated.
matteValue_t matte_value_object_access_string(matteStore_t *, matteValue_t, const matteString_t *);

/// Convenience function. Same as matte_value_object_access, except creates a 
/// temporary number object Bracket ([]) access is emulated. 
matteValue_t matte_value_object_access_index(matteStore_t *, matteValue_t, uint32_t);

/// Convenience function. Removes all number'keyed methods from the 
/// object table, resetting the key count back to zero for number keys.
/// the value is assumed to be a table object.
void matte_value_object_clear_number_keys_unsafe(matteStore_t *, matteValue_t);

/// If the value is an object, returns a new object with numbered 
/// keys pointing to the keys of the original objects. If not an object,
/// empty is returned
matteValue_t matte_value_object_keys(matteStore_t *, matteValue_t);

/// If the value is an object, returns a new object with numbered 
/// keys pointing to the values of the original objects. If not an object,
/// empty is returned.
matteValue_t matte_value_object_values(matteStore_t *, matteValue_t);


/// Returns the number of keys within the object.
/// If not an object, 0 is returned.
uint32_t matte_value_object_get_key_count(matteStore_t *, matteValue_t);

/// Returns the number of number keys within the object, ignoring keys of other types.
uint32_t matte_value_object_get_number_key_count(matteStore_t *, matteValue_t);

/// Inserts a numbered key into the object. If there are any number-keyed values 
/// whose keys are above the given index, those key-values are pushed up.
void matte_value_object_insert(matteStore_t *, matteValue_t, uint32_t key, matteValue_t val);

/// More efficient case of matte_value_object_insert with key == object numbere key count
void matte_value_object_push(
    matteStore_t * store, 
    matteValue_t v, 
    matteValue_t val
);


/// Sorts the number key contents of the object.
/// less is expected to be a function that takes 2 arguments, "a", and "b", and returns 
/// a comparison between them (-1 | 0 | 1)
void matte_value_object_sort_unsafe(matteStore_t *, matteValue_t, matteValue_t less);

/// Attempts to run a VM call for each key-value pair within the object.
void matte_value_object_foreach(matteStore_t *, matteValue_t object, matteValue_t function);


/// Attempts to set a key-value pair within the object.
/// Invokes assigner if present.
matteValue_t matte_value_object_set(
    matteStore_t *, 
    matteValue_t, 
    matteValue_t key, 
    matteValue_t value, 
    int isBracket
);


/// Same as matte_value_object_set() but is handy for string insertion
matteValue_t matte_value_object_set_key_string(matteStore_t *, matteValue_t, const matteString_t * key, matteValue_t value);



/// Performs a matte_value_object_set() with dot access for each 
/// key-value pair in srcTable. If srcTable is not a table, an error is raised.
///
void matte_value_object_set_table(matteStore_t *, matteValue_t, matteValue_t srcTable);





/// Sets the attributes set for this object, which describes how 
/// the object should respond to different operators and events.
/// This existing attributes object is replaced with this new object.
/// If this new object is the same as the original object, no change is made.
void matte_value_object_set_attributes(matteStore_t *, matteValue_t v, matteValue_t opObject);

/// Gets the attributes set for an object, assuming that the value is an object.
/// If the object has no attributes set, NULL is returned.
const matteValue_t * matte_value_object_get_attributes_unsafe(matteStore_t *, matteValue_t);

/// Gets the dynamic binding for an object, assuming that the value is an object.
/// If the object has no attributes set, the given object is returned.
matteValue_t matte_value_object_get_dynamic_binding_unsafe(matteStore_t *, matteValue_t);



/// Removes a key from an object if it exists. If the value is 
/// not an object or the key does not exist, no action is taken.
void matte_value_object_remove_key(matteStore_t *, matteValue_t, matteValue_t key);

/// Convenience function that calls matte_value_object_remove_key 
/// using a key value produced from a string
void matte_value_object_remove_key_string(matteStore_t *, matteValue_t, const matteString_t *);


/// Uniquely identifies the type (nonzero). Returns 0 if bad.
uint32_t matte_value_type_get_typecode(matteValue_t);


/// Returns whether the value is of the type given by the type value typeobj.
/// This takes into account inheritance.
int matte_value_isa(matteStore_t *, matteValue_t, matteValue_t typeobj);

/// Gets a Type object of the given value.
matteValue_t matte_value_get_type(matteStore_t *, matteValue_t);

/// Returns the result of a query lookup. This is usually either a directly 
/// computed value (such as ->cos, ->sin, etc) or a function (such as ->sort())
matteValue_t matte_value_query(matteStore_t * store, matteValue_t * v, matteQuery_t query);

/// Given a value type, returns the public name of it.
/// Every type has a name.
/// This is intended for use with custom types for more descriptive errors.
matteValue_t matte_value_type_name(matteStore_t *, matteValue_t);


/// Pushes a lock on the garbage collected. As long as one unpopped lock 
/// on the garbage collecter is active, no garbage collection will occur.
void matte_store_push_lock_gc(matteStore_t *);

/// Pops a lock on the garbage collector that was once pushed.
void matte_store_pop_lock_gc(matteStore_t *);

/// get built-in type object
const matteValue_t * matte_store_get_empty_type(matteStore_t *);
const matteValue_t * matte_store_get_boolean_type(matteStore_t *);
const matteValue_t * matte_store_get_number_type(matteStore_t *);
const matteValue_t * matte_store_get_string_type(matteStore_t *);
const matteValue_t * matte_store_get_object_type(matteStore_t *);
const matteValue_t * matte_store_get_function_type(matteStore_t *);
const matteValue_t * matte_store_get_type_type(matteStore_t *);
const matteValue_t * matte_store_get_any_type(matteStore_t *);









/*
    The following functions are not useful for standard users; 
    these functions should generally be avoided without knowing 
    their exact nature.
*/


/// Activates the closure references for the function, preparing it for calling.
/// Referrables are the shadow references held by the function from local 
/// objects and values. In other languages these may be referred to as "upvalues".
/// NOTE: values are NOT copied, but are transfered and then linked 
/// to the function. The callers references are transfered (moved)
///
/// This is normally not needed by user code.
void matte_value_object_function_activate_closure(matteStore_t *, matteValue_t v, matteArray_t * refs);


/// return whether the function was activated.
///
/// This is normally not needed by user code.
int matte_value_object_function_was_activated(matteStore_t *, matteValue_t v);



/// Directly sets a referrable value based on index with no bounds checking.
///
/// This is normally not needed by user code.
void matte_value_object_function_set_closure_value_unsafe(
    matteStore_t *, 
    matteValue_t func, 
    uint32_t index, 
    matteValue_t newVal
);

/// Directly gets a referrable value based on index with no bounds checking.
///
/// This is normally not needed by user code.
matteValue_t * matte_value_object_function_get_closure_value_unsafe(matteStore_t *, matteValue_t v, uint32_t);


/// Updates a value that this function captured but does not own.
///
/// This is normally not needed by user code.
void matte_value_set_captured_value(matteStore_t *, matteValue_t v, uint32_t index, matteValue_t val);

/// Gets a value captured by the function. If not a function, returns NULL
///
/// This is normally not needed by user code.
matteValue_t * matte_value_get_captured_value(matteStore_t *, matteValue_t, uint32_t index);

/// Performs the "preflight" check, making sure all arguments match 
/// the function's typestrict signature. args order corresponds 
/// to the stub's name argument list.
/// assumes: 
///  - the first arg is a function 
///  - the function is typestrict
///  - the arguments given match the bytecode stubs number of arguments
///
/// This is normally not needed by user code.
int matte_value_object_function_pre_typecheck_unsafe(matteStore_t *, matteValue_t, const matteArray_t * args);

/// Checks the functions typestrict signature to ensure the 
/// output is the correct type.
/// Assumes the first arg is a function and that the function is typestrict.
///
/// This is normally not needed by user code.
void matte_value_object_function_post_typecheck_unsafe(matteStore_t *, matteValue_t, matteValue_t);

/// Given that the value is an object, DIRECTLY sets a numbered-index value, bypassing 
/// the attribute overrides and any safety checks.
///
/// This is normally not needed by user code.
void matte_value_object_set_index_unsafe(matteStore_t *, matteValue_t, uint32_t i, matteValue_t value);

/// Gets the string representing the dynamic bind token. 
///
/// This is normally not needed by user code.
matteValue_t matte_store_get_dynamic_bind_token(matteStore_t *);

/// Updates another frame of the garbage collection routine.
/// This is normally called for you frequently.
/// Per call, this softly promises to "not hang too long"
/// and allow continuous execution as much as reasonably
/// possible. Despite this, the amount of time spent 
/// within this function can and will vary based on runtime factors.
///
/// The default operation time is not to exceed 3.5ms per call.
///
/// This is normally not needed by user code.
void matte_store_garbage_collect(matteStore_t *);


#endif
