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




typedef struct matteVM_t matteVM_t;


matteVM_t * matte_vm_create();

void matte_vm_destroy(matteVM_t*);

// Returns the heap owned by the VM.
matteHeap_t * matte_vm_get_heap(matteVM_t *);

// Adds an array of matteBytecodeStub_t * to the vm.
// Ownership of the stubs is transferred.
void matte_vm_add_stubs(const matteArray_t *);

// Runs the root functional stub of the file
// The value result of stub is returned. Empty if no result.
//
// A function object is created as the toplevel for the 
// root stub functional; the function is then run.
//
// This is equivalent to pushing the args onto the stack and 
// inserting a CAL instruction.
matteValue_t matte_vm_run_stub(
    matteVM_t *, 
    uint16_t fileid, 
    const matteArray_t * args
);


// Calls a function and evaluates its result immediately.
matteValue_t matte_vm_call(
    matteVM_t *,
    matteValue_t function,
    const matteArray_t * args
);

// raises an error
void matte_vm_raise_error(matteVM_t *, matteValue_t);

// raises an error string
void matte_vm_raise_error_string(matteVM_t *, const matteString_t *);



// Represents a stack frame within the current execution of the stub.
// The validity of the matteValue_t * values is only granted as so 
// until the next matte_vm call.
typedef struct {
    // the stub of the current function
    // If the stackframe is invalid, this will be NULL and no other values 
    // will be valid.
    const matteBytecodeStub_t * stub;
    // current instruction index within the stub
    uint32_t pc;
    // contextualized, human-readable string of the name of the running 
    // function. It is not always possible to get an accurate name,
    // so this is often a "best guess". Meant for debugging.
    matteString_t * prettyName;


    // function object of the stackframe
    matteValue_t * context;
    // array of matteValue_t values passed as args to the function
    matteArray_t * arguments;
    // array of matteValue_t values local to the function
    matteArray_t * locals;
    // array of matteValue_t values that were captured by this function
    matteArray_t * captured;
    
    
    // working array of values utilized by this function. (matteValue_t)
    matteArray_t * valueStack;
    
} matteVMStackFrame_t;


// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i);





#endif
