MATTE_EXT_FN(matte_cli__system_system) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("system() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("system() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_boolean(
        &out,
        system(matte_string_get_c_str(str)) == 0
    );


    return out;
}


MATTE_EXT_FN(matte_cli__system_exit) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("exit() requires at least one argument"));
        return matte_heap_new_value(heap);
    }
    exit(matte_value_as_number(matte_array_at(args, matteValue_t, 0)));
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_cli__system_init_rand) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    srand(time(NULL));
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_cli__system_next_rand) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(&out, (rand() / (double)(RAND_MAX)));    
    return out;
}

