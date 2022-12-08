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
#include "matte_heap.h"
#include "matte_opcode.h"



typedef struct matteVM_t matteVM_t;



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


typedef  uint8_t * (*matteImportFunction_t)(
    matteVM_t *,
    const matteString_t * importPath,
    uint32_t * preexistingFileID,
    uint32_t * dataLength,
    void * usrdata
);

#define MATTE_VM_DEBUG_FILE 0xfffffffe



matteVM_t * matte_vm_create();

void matte_vm_destroy(matteVM_t*);

// Returns the heap owned by the VM.
matteHeap_t * matte_vm_get_heap(matteVM_t *);

// Adds an array of matteBytecodeStub_t * to the vm.
// Ownership of the stubs is transferred.
void matte_vm_add_stubs(matteVM_t *, const matteArray_t *);

// Sets the implementation for the import function.
void matte_vm_set_import(
    matteVM_t * vm,
    matteImportFunction_t, 
    void * userData
);


// Returns the default call for import. This is useful when 
// setting an user import that has behavior happen when importing 
// but doesnt necessarily deviate from standard behavior.
matteImportFunction_t matte_vm_get_default_import();


// Expands a given filename as if it were from an import 
// call. This does not impact the import path chain.
// The returned path is a fully-qualified path.
// The result should be cleaned up by the caller.
matteString_t * matte_vm_import_expand_path(
    matteVM_t * vm, 
    const matteString_t * module
);

// Gets the current import path.
const matteString_t * matte_vm_get_import_path(matteVM_t * vm);

// Performs an import, which evaluates the source at the given path 
// and returns its value, as if calling import() within 
// source.
matteValue_t matte_vm_import(
    matteVM_t *, 
    const matteString_t * path, 
    matteValue_t parameters
);



// Runs the root functional stub of the file
// The value result of stub is returned. Empty if no result.
//
// A function object is created as the toplevel for the 
// root stub functional; the function is then run.
//
// Optionally, an importPath may be provided, which can 
// be used a current directory for any import() calls within this 
// fileid. Once the function returns, the import path is 
// popped from the import path stack.
//
//
// This is equivalent to pushing the args onto the stack and 
// inserting a CAL instruction.
matteValue_t matte_vm_run_fileid(
    matteVM_t *, 
    uint32_t fileid, 
    matteValue_t parameters,
    const matteString_t * importPath
);


// Calls a function and evaluates its result immediately.
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


    // array object that holds argumentsw and locals.
    // Retrieving a referrable safely incolves copying a 
    // reference to this. This guarantees access of the value past 
    // the lifetime of this function call when applicable
    matteValue_t referrable;
    uint32_t referrableSize;


    
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

// Functions for a built-in references. These are locked into the heap 
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
