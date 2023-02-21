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
MATTE_EXT_FN(matte_utility__system) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "system() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_boolean(
        heap, 
        &out,
        system(matte_string_get_c_str(str)) == 0
    );


    return out;
}


MATTE_EXT_FN(matte_utility__exit) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    exit(matte_value_as_number(heap, args[0]));
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_utility__os) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_string(heap, &v, MATTE_VM_STR_CAST(vm, "unix-like"));
    return v;
}

static void matte_system__utility(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_system"),   1, matte_utility__system, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_exit"),     1, matte_utility__exit, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_os"),     0, matte_utility__os, NULL);
}
