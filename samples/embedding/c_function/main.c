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
#include <matte_store.h>
#include <matte_string.h>

#include <stdio.h>

/*
    Example showing how to embed a C function 
    and have it called within the Matte VM


*/



matteValue_t c_function(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    // the Matte Store interfaces with internal values.
    matteStore_t * store = matte_vm_get_store(vm);
    
    // we dont know if the arguments are actually strings,
    // so we only continue if they really are strings.
    if (matte_value_type(args[0]) != MATTE_VALUE_TYPE_STRING ||
        matte_value_type(args[1]) != MATTE_VALUE_TYPE_STRING) {

        // signal an error in the VM and bail        
        matte_vm_raise_error_cstring(vm, "Inputs MUST be strings!");

        // external functions ALWAYS need to return a matte value,
        // even in error.
        return matte_store_new_value(store);
    }
    
    
    // now that we know we have 2 strings, we can safely pull the real 
    // strings from them. This function is labeled as "unsafe" because 
    // it doesnt do any error checking for you in the case that it isnt 
    // a string value. This is why we check ourselves ahead of time.
    const matteString_t * arg0_str = matte_value_string_get_string_unsafe(store, args[0]);
    const matteString_t * arg1_str = matte_value_string_get_string_unsafe(store, args[1]);
    
    
    // the purpose of the C function is to append these 2 strings,
    // so lets do it!
    matteString_t * output = matte_string_clone(arg0_str);
    matte_string_concat(output, arg1_str);
    
    
    // now we need to convert this string into a matte value.
    matteValue_t outputValue = matte_store_new_value(store);
    matte_value_into_string(store, &outputValue, output);
    
    // now we can free the string. It's copied and interned into the store, 
    // so this string is no longer needed.
    matte_string_destroy(output);
    
    // return the represented value;
    return outputValue;
};



int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be manually set 
    // through the Matte VM interface functions.
    matte_set_io(m, NULL, NULL, NULL);
    

    // Assigns a C function to an external name 
    // within the VM.     
    matte_add_external_function(
        m,
        "c_function_id",
        c_function,
        NULL,
        
        // argument names
        "first",
        "second",
        NULL
    );
    
    // run a script that just pulls the external function 
    // and runs it with some arguments.
    matteValue_t result = matte_run_source(
        m,
            // Gets the external function of the given ID 
            // throws an error if it fails.
            // When called, a subsequent call to retrieve 
            // the external function will fail. This is 
            // for security reasons.
            "@:f = getExternalFunction(name:'c_function_id');"
            
            // runs it and returns the result
            // If you change any of the arguments to 
            // non-strings, you will observe an error.
            // Try it!
            "return f(first:'Hello, ', second:'World!');"
    );

    printf("%s\n", matte_introspect_value(m, result));
}

