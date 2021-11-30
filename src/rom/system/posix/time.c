MATTE_EXT_FN(matte_time__sleepms) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Sleep requires a number argument"));
        return matte_heap_new_value(heap);
    }


    usleep(1000*matte_value_as_number(heap, matte_array_at(args, matteValue_t, 0)));
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


static void matte_system__time(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_sleepms"),   1, matte_time__sleepms, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_getticks"),    0, matte_time__getticks, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::time_time"),   0, matte_time__time, NULL);

}
