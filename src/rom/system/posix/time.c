MATTE_EXT_FN(matte_cli__system_sleepms) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Sleep requires a number argument"));
        return matte_heap_new_value(heap);
    }


    usleep(1000*matte_value_as_number(matte_array_at(args, matteValue_t, 0)));
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_cli__system_getticks) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    struct timeval t; 
    gettimeofday(&t, NULL);

    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(&v, t.tv_sec*1000LL + t.tv_usec/1000);
    return v;
}




MATTE_EXT_FN(matte_cli__system_time) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    time_t t = time(NULL);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(&v, t);
    return v;
}



