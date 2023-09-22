/*
Copyright (c) 2021, Johnathan Corkery. (jcorkery@umich.edu)
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
#ifndef H_MATTE__VM__INCLUDED
#define H_MATTE__VM__INCLUDED

#include <stdint.h>
typedef struct matteArray_t matteArray_t;
typedef struct matteBytecodeStub_t matteBytecodeStub_t;
typedef struct matteVMStackFrame_t matteVMStackFrame_t;
#include "matte_store.h"
#include "matte_opcode.h"



typedef struct matteVM_t matteVM_t;
typedef struct matte_t matte_t;


typedef enum {
    // Event from when the running VM 
    MATTE_VM_DEBUG_EVENT__LINE_CHANGE,
    // Event that a general error is raised from the software.
    // This event is signaled right when the error is raised, so it may 
    // be caught by the software.
    MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
    
    // User code may call the built-in "breakpoint()" function 
    // When called and the debug context is enabled, a debug event 
    // will be fired when it is reached. Otherwise, the "breakpoint()" 
    // call is ignored.
    MATTE_VM_DEBUG_EVENT__BREAKPOINT
} matteVMDebugEvent_t;


// The import handler function
//
// The import function returns the fileID 
// represented by this name.
// The most common use of this is to load in the corresponding 
// import bytecode stubs at the same time.
//
typedef uint32_t (*matteImportFunction_t)(
    // The VM instance
    matteVM_t *,
    // The name of the module being requested
    const matteString_t * name,
    
    void * usrdata
);

#define MATTE_VM_DEBUG_FILE 0xfffffffe



matteVM_t * matte_vm_create(matte_t *);

void matte_vm_destroy(matteVM_t*);

// Returns the store owned by the VM.
matteStore_t * matte_vm_get_store(matteVM_t *);

// Adds an array of matteBytecodeStub_t * to the vm.
// Ownership of the stubs is transferred (but not the array)
void matte_vm_add_stubs(matteVM_t *, const matteArray_t *);

// Sets the implementation for the import function.
void matte_vm_set_import(
    matteVM_t * vm,
    matteImportFunction_t, 
    void * userData
);



// Performs an import, which evaluates the source of the given name 
// and returns its value, as if calling import() within 
// source. If an import was already performed on that name,
// the pre-computed value of the previous import is returned.
matteValue_t matte_vm_import(
    matteVM_t *, 
    const matteString_t * name, 
    matteValue_t parameters
);



// Runs the root functional stub of the file
// The value result of stub is returned. Empty if no result.
//
// A function object is created as the toplevel for the 
// root stub functional; the function is then run.
//
//
//
// This is equivalent to pushing the args onto the stack and 
// inserting a CAL instruction.
// The value is owned and reserved by the VM,
// so it shoud not be recycled.
matteValue_t matte_vm_run_fileid(
    matteVM_t *, 
    uint32_t fileid, 
    matteValue_t parameters
);



/// Functions are called immediately and only return once computation is finished.
/// This includes any sub-calls to matte_call / matte_vm_call. The return value 
/// of the function is returned. 
matteValue_t matte_vm_call(
    matteVM_t *,
    matteValue_t function,
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
);

// Attempts to just-in-time compile and run source
// at the given scope. If failure occurs, whether in running or 
// in compilation, the error function will be run set and the empty 
// value will always be returned. Else, the result of the 
// expression will be returned. In the case of error,
// The VM's error stack will not be affected.
//
// The source is treated as if it were a full function, thus the result 
// returned is whatever your function returns. In other words,
// all the lines of course given should be function statements,
// and if a meaningful value is desired, the return or when statements 
// should be used.
//
// Depending on the chosen scope, variables that arent declared 
// within the given source will attempt to be linked to 
// the variables accesible within the stackframe scope.
// This is dependent on the functions being compiled with 
// "named references" which are purely used for debugging 
// purposes such as this. 
//
// Because named references are used for debugging, there is 
// a possibility that the original source code for the functions 
// running in scope were not cmpiled with valid or accurate names.
// This will cause the debug source to fail to run and generate an 
// error. As such, this is best for debugging your own source 
// rather than inspecting external / unknown sources.
//
matteValue_t matte_vm_run_scoped_debug_source(
    matteVM_t *,
    const matteString_t * expression,
    int callstackIndex,
    void(*onError)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *),
    void * onErrorData
);    

// raises an error
void matte_vm_raise_error(matteVM_t *, matteValue_t);

// raises an error string
void matte_vm_raise_error_string(matteVM_t *, const matteString_t *);

// convenience function that raises an error string from a c string 
void matte_vm_raise_error_cstring(matteVM_t *, const char * );

int matte_vm_pending_message(matteVM_t *);


// Represents a stack frame within the current execution of the stub.
// The validity of the matteValue_t * values is only granted as so 
// until the next matte_vm call.
struct matteVMStackFrame_t {
    // the stub of the current function
    // If the stackframe is invalid, this will be NULL and no other values 
    // will be valid.
    const matteBytecodeStub_t * stub;

    // function object of the stackframe.
    // holds captured values.
    matteValue_t context;

    // current instruction index within the stub
    uint32_t pc;
    // contextualized, human-readable string of the name of the running 
    // function. It is not always possible to get an accurate name,
    // so this is often a "best guess". Meant for debugging.
    matteString_t * prettyName;


    // copy of the referrable list owned by the context function 
    // Is available for quick access, but is owned by the context function
    // NOTE: these values are ONLY representative of the context referrables 
    // at the ACTIVATION of the stack frame. Due to the nature of closures 
    // they will almost always mutate. The only metric that is guaranteed is 
    // the size of the referrable list, which is static.
    matteArray_t *  referrables;


    
    // working array of values utilized by this function. (matteValue_t)
    matteArray_t * valueStack;
    

    // Some parts of algorithms may want to start the current calling stack 
    // frame function from the start. If restartCondition is not-null, it 
    // will be run with its data. If it returns TRUE, pc will be set back to 
    // zero and the function will restart without having to go 
    // through normal destruction/initialization code for recalling a function.
    // The to-be result of the calling function is passed in. It is discarded if 
    // TRUE is returned.
    int (*restartCondition)(matteVM_t * vm, matteVMStackFrame_t *, matteValue_t result, void * data);
    void * restartConditionData;
};




// Gets the requested stackframe. 0 is the currently running stackframe.
// If an invalid stackframe is requested, an error is raised.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i);

// Returns the number of valid stackframes.
uint32_t matte_vm_get_stackframe_size(const matteVM_t *);

// Gets a pointer to the special referrable value in question.
// If none, NULL is returned.
// Can raise error.
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



// Creates a native handle to a Matte.Core.MemoryBuffer.
// This is intended to make it easy and efficient for 
// usercode to generate a MemoryBuffer instance from 
// native data.
// Note that what this returns is the native handle and NOT 
// a MemoryBuffer instance. To get a normal MemoryBuffer instance,
// - Import Matte.Core.MemoryBuffer
// - Instantiate a new buffer
// - Use the MemoryBuffer.bindNative() function and pass this result as its handle argument
matteValue_t matte_vm_create_memory_buffer_handle_from_data(
    matteVM_t * vm,
    const uint8_t * data,
    uint32_t size
);


// As the inverse of matte_vm_create_memory_buffer_handle_from_data(),
// Retrieves a raw data buffer for a handle of a MemoryBuffer instance.
//
// This can be retrieved from a MemoryBuffer instance by accessing the 
// .handle getter.
// This function can throw a VM error if the buffer is inaccessible.
uint8_t * matte_vm_get_memory_buffer_handle_raw_data(
    matteVM_t * vm,
    matteValue_t handle,
    uint32_t * size
);



// Adds an external function.
// In the script context: calling getExternalFunction() with the string identifier 
// given will return a function object that, when called, calls this C function.
//
// The args are given in the order that you specific in the 
// argNames array. The number of values within the C function 
// will always match the number of argNames for the function.
//
// If the calling context does not bind an argument, the slot within the 
// called C function will be an empty value.
void matte_vm_set_external_function(
    matteVM_t * vm, 
    const matteString_t * identifier,
    const matteArray_t * argNames,
    matteValue_t (*)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),
    void * userData
);


// Same as matte_vm_set_external_function, but automatically assigns 
// default parameter name bindings rather than requiring you to provide them.
// This is convenient if you are developing a middleware interface 
// to some lower-level functionality where a rea, user-facing 
// API will rest on top of it.
//
// The arguments are always a-z, a being argument 0, b 1, etc.
void matte_vm_set_external_function_autoname(
    matteVM_t * vm, 
    const matteString_t * identifier,
    uint8_t nArgs,
    matteValue_t (*)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),
    void * userData
);



// Gets a new function object that, when called, calls the registered 
// C function.
matteValue_t matte_vm_get_external_function_as_value(
    matteVM_t * vm,
    const matteString_t * identifier
);

// Functions for a built-in references. These are locked into the store 
matteValue_t * matte_vm_get_external_builtin_function_as_value(
    matteVM_t * vm,
    matteExtCall_t ext
);


// Gets an unused fileid
uint32_t matte_vm_get_new_file_id(matteVM_t * vm, const matteString_t * name);

// Gets a script name by fileID. If none is associated, NULL is 
// returned.
const matteString_t * matte_vm_get_script_name_by_id(matteVM_t * vm, uint32_t fileid);


uint32_t matte_vm_get_file_id_by_name(matteVM_t * vm, const matteString_t * name);

// gets a debug event callback.
void matte_vm_set_debug_callback(
    matteVM_t * vm,
    void(*)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *),
    void *
);
// gets a debug event callback.
void matte_vm_set_unhandled_callback(
    matteVM_t * vm,
    void(*)(matteVM_t *, uint32_t file, int lineNumber, matteValue_t value, void *),
    void *
);

// Adds a function to be called right when the VM is destroyed.
void matte_vm_add_shutdown_callback(
    matteVM_t * vm,
    void(*)(matteVM_t *, void *),
    void *
);

// Sets a handler for the built-in print function.
// This is most convenient for debugging. This default behavior 
// is to print the string to stdout.
void matte_vm_set_print_callback(
    matteVM_t * vm,
    void(*)(matteVM_t *, const matteString_t *, void *),
    void *
);

/// Returns a temporary string built from the given cstring 
/// It is meant as a convenience function, but it has the following 
/// restrictions:
///     - This must only be used on one thread at a time. It is not thread-safe.
///     - The reference fizzles after subsequent calls to this function. 
///       The string must only be used for quick operations. 
///
/// If your use case does not adhere to these, you should 
/// allocate a new string instead.
const matteString_t * matte_vm_cstring_to_string_temporary(matteVM_t * vm, const char * );

/// A shorter form of matte_vm_cstring_to_string_temporary().
///
#define MATTE_VM_STR_CAST(__vm__, __cstr__) matte_vm_cstring_to_string_temporary(__vm__, __cstr__)

#endif
