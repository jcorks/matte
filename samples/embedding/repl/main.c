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
#include <matte.h>
#include <matte_vm.h>
#include <matte_heap.h>
#include <matte_string.h>

#include <stdio.h>
#include <string.h>

/*
    Example showing a hypothetical Read, Evaluate, Print, Loop 
    program using Matte.

    This example will only allow running of expressions, which 
    means new variable creation is impossible. 
    
    Instead, there will be a _ object that can be modified.

*/



// whether the repl should keep going
static int KEEP_GOING = 1;

// when a user calls "exit()" this signals the end of the repl.
static matteValue_t repl_exit(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    KEEP_GOING = 0;
    matteHeap_t * heap = matte_vm_get_heap(vm);
    
    
    
    return matte_heap_new_value(heap);
};


int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be manually set 
    // through the Matte VM interface functions.
    matte_set_io(m, NULL, NULL, NULL);
    
    matteVM_t * vm = matte_get_vm(m);
    matteHeap_t * heap = matte_vm_get_heap(vm);
    
    // since the repl cannot make any new variables
    matteValue_t state = matte_heap_new_value(heap);

    // turns the value into a newly created object.
    matte_value_into_new_object_ref(heap, &state);
    
    // tells the heap that the object SHOULD NOT be 
    // garbage collected under any circumstance.
    // This is important for values that live outside the VM
    matte_value_object_push_lock(heap, state);
    
    
    // register the repl_exit
    matte_add_external_function(m, "repl_exit", repl_exit, NULL, NULL);
    
    
    // create the exit function
    matteValue_t exitFunc = matte_run_source(m, "return getExternalFunction(name:'repl_exit');");
    
    printf("Matte REPL.\n");
    printf("Johnathan Corkery, 2023\n\n"); //<- That's me!
    
    while(KEEP_GOING) {
        #define REPL_LINE_LIMIT 1024
        // print header
        printf(">> ");
        
        // get stdin line.
        char line[REPL_LINE_LIMIT+1];
        fgets(line, REPL_LINE_LIMIT, stdin); 
        
        
        // create source from that line
        // that consists of a function with 2 arguments, the exit function 
        // and a _ state object.
        matteString_t * source = matte_string_create_from_c_str("return ::(exit, store){return (%s);};", line);
        matteValue_t nextFunc = matte_run_source(m, matte_string_get_c_str(source));
        matte_string_destroy(source);


        // If it was successful, it will be non-empty.
        // If its empty, there was likely a compiler error.
        if (matte_value_type(nextFunc) != MATTE_VALUE_TYPE_EMPTY) {
            // actually run that function now that its compiled        
            matteValue_t result = matte_call(
                m,
                nextFunc,
                "exit", exitFunc,
                "store", state,
                NULL
            );
            printf("%s\n\n", matte_introspect_value(m, result));        
        }
    }    
    
    return 0;

}

