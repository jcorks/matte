


typedef struct {
    pthread_t pthreadID;

    int workerid;
    int state;

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
    
    
    // If an error occurs, this will be non-null.
    matteString_t * errorString;

} MatteWorkerChildInfo;

typedef struct {
    matteString_t * message;
    int id; // -1 for parent, else thread id
} MatteWorkerPendingMessage;

typedef struct {
    matteString_t * inputString;

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
    
    // State to indicate to parent a state change.
    int * stateRef;



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
} MatteAsyncInfo;

typedef enum {
    MWAS_RUNNING,
    MWAS_FINISHED,
    MWAS_FAILED,
    MWAS_UNKNOWN
} MatteWorkerAsyncState;


// TLS 
__thread MatteAsyncInfo asyncSelf = {};

typedef struct {
    // parent's VM
    matteVM_t * fromVM;

    // import path to the source. New thread frees
    char * from;
    
    // input string from parent
    matteString_t * input;

    // pointer to the parent-owned result location;    
    matteString_t ** result;
    
    // pointer to the parent-owned errorString location;
    matteString_t ** errorString;
    
    // pointer to parent-owned state ref (within corresponding MatteWorkerChildInfo)
    int * stateRef;
} MatteAsyncStartData;


static MatteWorkerChildInfo * find_child(int id) {
    uint32_t i;
    uint32_t len = matte_array_get_size(asyncSelf.children);
    for(i = 0; i < len; ++i) {
        if (matte_array_at(asyncSelf.children, MatteWorkerChildInfo *, i)->workerid == id) {
            return matte_array_at(asyncSelf.children, MatteWorkerChildInfo *, i);
        }    
    }
    return NULL;
}

static void matte_thread_compiler_error(const matteString_t * s, uint32_t line, uint32_t ch, void * d) {
    MatteAsyncStartData * startData = d;
    *startData->errorString = matte_string_create_from_c_str("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    *startData->stateRef = MWAS_FAILED;
}

static void matte_thread_error(
    matteVM_t * vm, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * d
) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteAsyncStartData * startData = d;
    if (val.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t s = matte_value_object_access_string(heap, val, MATTE_VM_STR_CAST(vm, "summary"));
        if (s.binID) {
            MatteAsyncStartData * startData = d;
            *startData->errorString = matte_string_create_from_c_str(
                "Unhandled error: \n%s\n", 
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, s)), 
                matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
                lineNumber
            );
            *startData->stateRef = MWAS_FAILED;        
            return;
        }
    }
    
    *startData->errorString = matte_string_create_from_c_str(
        "Unhandled error (%s, line %d)\n", 
        matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
        lineNumber
    );
    *startData->stateRef = MWAS_FAILED;        
}


static void * matte_thread(void * userData) {
    
    MatteAsyncStartData * startData = userData;
    uint32_t lenBytes;
    *startData->stateRef = MWAS_UNKNOWN;
    asyncSelf.inputString = startData->input;
    uint8_t * src = dump_bytes(startData->from, &lenBytes);

    if (!src || !lenBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Could not read from file %s", src);        
        *startData->errorString = str;
        *startData->stateRef = MWAS_FAILED;
        
        free(startData);  
        return NULL;
    }

    uint32_t outByteLen;
    uint8_t * outBytes = matte_compiler_run(
        src,
        lenBytes,
        &outByteLen,
        matte_thread_compiler_error,
        startData
    );

    free(src);
    if (!outByteLen || !outBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Couldn't compile source %s", src);
        *startData->errorString = str;
        free(src);
        *startData->stateRef = MWAS_FAILED;
        free(startData);  
        return NULL;
    }



    
    matte_t * m = matte_create();
    matteVM_t * vm = matte_get_vm(m);
    matteHeap_t * heap = matte_vm_get_heap(vm);

    // first compile all and add them 
    int FILEIDS = matte_vm_get_new_file_id(vm, MATTE_VM_STR_CAST(vm, startData->from));

    

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(FILEIDS, outBytes, outByteLen);
    free(outBytes);
    matte_vm_add_stubs(vm, arr);
    matte_array_destroy(arr);


    matte_vm_set_unhandled_callback(
        vm,
        matte_thread_error,
        startData
    );

    matteArray_t * args = matte_array_create(sizeof(matteValue_t));        
    *startData->stateRef = MWAS_RUNNING;
    matteValue_t v = matte_vm_run_script(vm, FILEIDS, args);

    

    if (v.binID == MATTE_VALUE_TYPE_STRING) {
        *startData->result = matte_string_clone(matte_value_string_get_string_unsafe(heap, v));
        matte_heap_recycle(heap, v);        
    } else {
    }

    matte_array_destroy(args);
    matte_destroy(m);
    if (!*startData->errorString)
        *startData->stateRef = MWAS_FINISHED;  
    free(startData);  
    return NULL;
}


static int matte_async_next_id() {
    int i;
    int len = (int)matte_array_get_size(asyncSelf.children);
    
    for(i = 0; i < len; ++i) {
        if (matte_array_at(asyncSelf.children, MatteWorkerChildInfo, i).workerid == asyncSelf.workeridpool) {
            asyncSelf.workeridpool++;
            i = -1;
        }
    }
    
    return asyncSelf.workeridpool++;
}


MATTE_EXT_FN(matte_async__start) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    // source ensures strings
    matteValue_t path  = matte_array_at(args, matteValue_t, 0);
    matteValue_t input = matte_array_at(args, matteValue_t, 1);

    MatteWorkerChildInfo * ch = calloc(1, sizeof(MatteWorkerChildInfo));
    MatteAsyncStartData * init = calloc(1, sizeof(MatteAsyncStartData));
    init->errorString = &ch->errorString;
    init->from = strdup(matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, path))));
    init->input = matte_string_clone(matte_value_string_get_string_unsafe(heap, input));
    init->stateRef = &(ch->state);
    init->fromVM = vm;
    ch->workerid = matte_async_next_id();
    
    matte_array_push(asyncSelf.children, ch);
    pthread_create(&(ch->pthreadID), NULL, matte_thread, init);
    
    matteValue_t id = matte_heap_new_value(heap);
    matte_value_into_number(heap, &id, ch->workerid);
    return id;
}


MATTE_EXT_FN(matte_async__state) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    int id = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 0));
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_heap_new_value(heap);
    }
    
    matteValue_t result = matte_heap_new_value(heap);
    matte_value_into_number(heap, &result, ch->state);
    return result;   
}

MATTE_EXT_FN(matte_async__input) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (asyncSelf.inputString) {
        matteValue_t v = matte_heap_new_value(heap);
        matte_value_into_string(heap, &v, asyncSelf.inputString);
        return v;
    } else {
        return matte_heap_new_value(heap);
    }
    
}

MATTE_EXT_FN(matte_async__result) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    int id = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 0));
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_heap_new_value(heap);
    }

    if (ch->result) {
        matteValue_t v = matte_heap_new_value(heap);
        matte_value_into_string(heap, &v, ch->result);
        return v;
    } else {
        return matte_heap_new_value(heap);
    }
}


MATTE_EXT_FN(matte_async__error) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    int id = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 0));
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_heap_new_value(heap);
    }

    if (ch->errorString) {
        matteValue_t v = matte_heap_new_value(heap);
        matte_value_into_string(heap, &v, ch->errorString);
        return v;
    } else {
        return matte_heap_new_value(heap);
    }
}

MATTE_EXT_FN(matte_async__sendmessage) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_async__nextmessage) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    return matte_heap_new_value(heap);    
}

static void matte_system__async_cleanup(matteVM_t * vm) {
    matte_array_destroy(asyncSelf.messagesReceived);
    matte_array_destroy(asyncSelf.messagesPending);
    matte_array_destroy(asyncSelf.children);
}

static void matte_system__async(matteVM_t * vm) {
    asyncSelf.messagesReceived = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf.messagesPending  = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf.children = matte_array_create(sizeof(MatteWorkerChildInfo*));
    

    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_start"),   2, matte_async__start,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_state"),   1, matte_async__state,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_input"),   0, matte_async__input,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_result"),  1, matte_async__result,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_error"),   1, matte_async__error,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_sendmessage"),   2, matte_async__sendmessage,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_nextmessage"),   0, matte_async__nextmessage,   NULL);

    matte_vm_add_shutdown_callback(vm, matte_system__async_cleanup, NULL);
}
