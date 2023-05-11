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

/*
    Simple example that shows how to run a Matte function 
    within C.
*/

#include <matte.h>
#include <matte_vm.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be set here
    matte_set_io(m, NULL, NULL, NULL);
    
    
    // Runs a script that returns a function as a value.
    matteValue_t matteFunction = matte_run_source(m, 
        "@:add ::(a, b) {"
        "  return a + b;"
        "};"
        "return add;"
    );
    
    
    // prepare arguments to run the function
    matteVM_t * vm = matte_get_vm(m);
    matteStore_t * store = matte_vm_get_store(vm);

    matteValue_t arg0 = matte_store_new_value(store);
    matteValue_t arg1 = matte_store_new_value(store);
    
    matte_value_into_number(store, &arg0, 12);
    matte_value_into_number(store, &arg1, 34);


    // call the function, binding arguments to their proper names
    matteValue_t result = matte_call(
        m, 
        matteFunction,
        "a", arg0,
        "b", arg1,
        NULL
    );
    
    printf("%s\n", matte_introspect_value(m, result));

    // destroy the instance and cleanup
    matte_destroy(m); 


    return 0;
}
