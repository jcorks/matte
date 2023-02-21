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
MATTE_EXT_FN(matte_time__sleepms) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    usleep(1000*matte_value_as_number(heap, args[0]));
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_time__getticks) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    struct timeval t; 
    gettimeofday(&t, NULL);

    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(heap, &v, t.tv_sec*1000LL + t.tv_usec/1000);
    return v;
}




MATTE_EXT_FN(matte_time__time) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    time_t t = time(NULL);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(heap, &v, t);
    return v;
}

MATTE_EXT_FN(matte_time__date) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    time_t now = time(NULL);
    struct tm * t = localtime(&now);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &v);
    
    matteValue_t key_day = matte_heap_new_value(heap);
    matteValue_t key_month = matte_heap_new_value(heap);
    matteValue_t key_year = matte_heap_new_value(heap);

    matteValue_t val_day = matte_heap_new_value(heap);
    matteValue_t val_month = matte_heap_new_value(heap);
    matteValue_t val_year = matte_heap_new_value(heap);
    
    
    matte_value_into_string(heap, &key_day, MATTE_VM_STR_CAST(vm, "day"));
    matte_value_into_string(heap, &key_month, MATTE_VM_STR_CAST(vm, "month"));
    matte_value_into_string(heap, &key_year, MATTE_VM_STR_CAST(vm, "year"));

    matte_value_into_number(heap, &val_day, t->tm_mday);
    matte_value_into_number(heap, &val_month, t->tm_mon + 1);
    matte_value_into_number(heap, &val_year, t->tm_year + 1900);
    
    matte_value_object_set(heap, v, key_day, val_day, 1);
    matte_value_object_set(heap, v, key_month, val_month, 1);
    matte_value_object_set(heap, v, key_year, val_year, 1);
    return v;
}



static void matte_system__time(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_sleepms"),   1, matte_time__sleepms, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_getticks"),    0, matte_time__getticks, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_time"),   0, matte_time__time, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_date"), 0, matte_time__date, NULL);

}
