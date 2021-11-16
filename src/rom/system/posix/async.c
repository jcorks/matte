


typedef struct {
    pthread_t pthreadID;

    int workerid;

    // NULL until the child 
    matteString_t * result;


    // Immediate pending slot for a message from a child to a parent.
    // always written by the child IFF its null 
    // always read by parent IFF its not null. Parent then writes 
    // NULL when its done.
    // While this struct symbolically refers to the child, it is owned by the parent.
    matteString_t * messageFromChildToParent;

    // Immediate pending slot for a message from a parent to a child 
    // always written by the parent IFF its null 
    // always read by the child IDD its not null
    // While this struct symbolically refers to the child, it is owned by the parent.
    matteString_t * messageFromParentToChild;

} MatteWorkerChildInfo;

typedef struct {
    matteString_t * message;
    int id; // -1 for parent, else thread id
} MatteWorkerPendingMessage;

typedef struct {
    matteString_t * inputString;
    // parent thread. may be
    MatteWorkerInfo * parent;

    // Immediate pending slot for a message from a child to a parent.
    // always written by the child IFF its null 
    // always read by parent IFF its not null. Parent then writes 
    // NULL when its done.
    // This struct refers to the child.
    matteString_t ** messageFromChildToParent;

    // Immediate pending slot for a message from a parent to a child 
    // always written by the parent IFF its null 
    // always read by the child IDD its not null
    // This struct refers to the child.
    matteString_t ** messageFromParentToChild;



    // pointer to the location where the 
    // parent is holding  the result
    matteString_t ** parentResult;

    // children threads. Holds MatteWorkerChildInfo*
    matteArray_t * children;

    // array of MatteWorkerPendingMessage. On update, these are collected 
    // from the parent and children. 
    matteArray_t * messagesReceived;

    // array of MatteWorkerPendingMessages that are waiting to be send to others.
    matteArray_t * messagesPending;

    // remember to check if current one is in use.
    // non-negative only.
    int workeridpool;
} MatteASyncInfo;

__thread MatteAsyncInfo asyncSelf = {};

static int matte_thread(void * userData) {

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
    FILEIDS = matte_vm_get_new_file_id(vm, MATTE_VM_STR_CAST(vm, from));

    

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
    
}





MATTE_EXT_FN(matte_async__create) {

}

static void matte_system__async(matteVM_t * vm) {


}
