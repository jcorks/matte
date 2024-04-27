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
    Simple example that runs matte source within a
    C++ executable.

*/

#include <matte.h>
#include <cstddef>
#include <matte_store.h>
#include <matte_vm.h>

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// sorts an input array object using std::sort
matteValue_t sortFunction(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t list = args[0];
    
    // only sort an array!
    if (matte_value_type(list) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_cstring(vm, "Can only C++ sort an array!");
        return matte_store_new_value(store);
    }
    
    std::vector<double> values;
    
    // retrieve the array and place contents within the vector
    size_t len = matte_value_object_get_number_key_count(store, list);
    for(size_t i = 0; i < len; ++i) {
        matteValue_t val = matte_value_object_access_index(store, list, i);
        
        // we are only sorting numbers, so error out if given things 
        // other than numbers.
        if (matte_value_type(val) != MATTE_VALUE_TYPE_NUMBER) {
            matte_vm_raise_error_cstring(vm, "Can only C++ sort an array of numbers!");
            return matte_store_new_value(store);
        }
        
        double valDouble = matte_value_as_number(store, val);
        values.push_back(valDouble);
    }
    
    
    // sort the numbers
    std::sort(values.begin(), values.end());


    // create a new object that serves as the output array.
    matteValue_t output = matte_store_new_value(store);
    matte_value_into_new_object_ref(store, &output);
    
    uint32_t index = 0;
    matteValue_t key = matte_store_new_value(store);
    matteValue_t value = matte_store_new_value(store);
    
    // for each vector value, place it in order inside 
    // the object by setting key values.
    for(auto it = values.begin(); it != values.end(); ++it) {
        matte_value_into_number(store, &key, index);
        matte_value_into_number(store, &value, *it);
        matte_value_object_set(store, output, key, value, 1);
        
        index++;
    }
    
    // return the sorted array
    return output;
}



int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be set here
    matte_set_io(m, NULL, NULL, NULL);
    
    // Assigns a C++ function to an external name 
    // within the VM.     
    matte_add_external_function(
        m,
        "c++_sort",
        sortFunction,
        NULL,
        
        // argument names
        "list",
        NULL
    );    
    
    // matte_run_source is a quick way to run raw 
    // source. The IO output function is used if an 
    // issue is encountered.
    // NOTE: there is a built-in function for sorting;
    // this example is for demonstration purposes only!
    matteValue_t result = matte_run_source(m,
        // input array 
        "@:array = [10, 40, 3, 6, 20, 450];"
        
        // Retrieve the external C++ sort function
        "@:sort = getExternalFunction(:'c++_sort');"
        
        // Call the sort and return the newly sorted array.
        "return sort(:array);"
    );
    
    // return the sorted array as an introspected string.
    std::cout << std::string(matte_introspect_value(m, result)) << std::endl;

    // destroy the instance and cleanup
    matte_destroy(m); 

    return 0;
}
