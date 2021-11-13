



static int matte_thread(matteVM_t * fromVM, const char * from, void * data) {
    uint32_t lenBytes;

    uint8_t * src = dump_bytes(from, &lenBytes);

    if (!src || !lenBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Could not read from file %s", src);

        matte_vm_raise_error_string(fromVM, str);
        matte_string_destroy(str);
        return;
    }

    uint32_t outByteLen;
    uint8_t * outBytes = matte_compiler_run(
        src,
        lenBytes,
        &outByteLen,
        NULL,
        NULL
    );

    free(src);
    if (!outByteLen || !outBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Couldn't compile source %s", src);
        matte_vm_raise_error_string(fromVM, str);
        matte_string_destroy(str);
        free(src);
        return;
    }



    
    matte_t * m = matte_create();
    matteVM_t * vm = matte_get_vm(m);
    matte_vm_add_system_symbols(vm, NULL, 0);
    matte_vm_add_thread_symbols(vm);


    // first compile all and add them 
    FILEIDS = matte_vm_get_new_file_id(vm, MATTE_STR_CAST(from));

    

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(FILEIDS, outBytes, outByteLen);
    free(outBytes);
    matte_vm_add_stubs(vm, arr);
    matte_array_destroy(arr);

    matteArray_t * arr = matte_array_create(sizeof(matteValue_t));        
    for(i = 0; i < len; ++i) {
        matteValue_t v = matte_vm_run_script(vm, FILEIDS, arr);
        const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(v));
        if (str && v.binID != 0) {
            printf("> %s\n", matte_string_get_c_str(str));
            
        } else {
            // output object was not string coercible.
        }
        matte_heap_recycle(v);
    }
    matte_array_destroy(arr);
    matte_destroy(m);
    */
}

MATTE_EXT_FN(matte_cli__system_threadstart) {

}



static void matte_system__thread(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::mbuffer_threadstart"), 2, matte_cli__system_threadstart, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_threadjoin"), 1, matte_cli__system_threadjoin, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_threadstate"), 1, matte_cli__system_threadstate, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_threadresult"), 1, matte_cli__system_threadresult, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_threadnextmessage"), 0, matte_cli__system_threadnextmessage, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_threadsendmessage"), 2, matte_cli__system_threadsendmesssage, NULL);


}
