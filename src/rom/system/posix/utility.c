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



MATTE_EXT_FN(matte_utility__init_rand) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    srand(time(NULL));
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_utility__next_rand) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, (rand() / (double)(RAND_MAX)));    
    return out;
}

static void matte_system__utility(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_system"),   1, matte_utility__system, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_initRand"), 0, matte_utility__init_rand, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_nextRand"), 0, matte_utility__next_rand, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::utility_exit"),     1, matte_utility__exit, NULL);
}
