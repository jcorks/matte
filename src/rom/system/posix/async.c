


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
    // State to indicate to parent a state change.
    int * stateRef;




    // children threads. Holds MatteWorkerChildInfo*
    matteArray_t * children;

    // array of MatteWorkerPendingMessage. On update, these are collected 
    // from the children. 
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
    matteString_t ** resultRef;
    
    // pointer to the parent-owned errorString location;
    matteString_t ** errorStringRef;
    
    // Immediate pending slot for a message from a child to a parent.
    // always written by the child IFF its null 
    // always read by parent IFF its not null. Parent then writes 
    // NULL when its done.
    // This struct refers to the child.
    matteString_t ** messageFromChildToParentRef;
    

    // Immediate pending slot for a message from a parent to a child 
    // always written by the parent IFF its null 
    // always read by the child IFF its not null
    // This struct refers to the child.
    matteString_t ** messageFromParentToChildRef;
    
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
    *startData->errorStringRef = matte_string_create_from_c_str("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
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
            *startData->errorStringRef = matte_string_create_from_c_str(
                "Unhandled error: \n%s\n", 
                matte_string_get_c_str(matte_value_string_get_string_unsafe(heap, s)), 
                matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
                lineNumber
            );
            *startData->stateRef = MWAS_FAILED;        
            return;
        }
    }
    
    *startData->errorStringRef = matte_string_create_from_c_str(
        "Unhandled error (%s, line %d)\n", 
        matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
        lineNumber
    );
    *startData->stateRef = MWAS_FAILED;        
}

static void on_async_print(matteVM_t * vm, const matteString_t * str, void * ud) {
    printf("%s\n", matte_string_get_c_str(str));
}


MATTE_EXT_FN(matte_asyncworker__send_message) {
    MatteAsyncStartData * startData = userData;
    if (!*startData->messageFromChildToParentRef)
         *startData->messageFromChildToParentRef = matte_string_clone(matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), args[0]));    

    return matte_heap_new_value(matte_vm_get_heap(vm));
}

MATTE_EXT_FN(matte_asyncworker__check_message) {
    MatteAsyncStartData * startData = userData;
    matteValue_t out = matte_heap_new_value(matte_vm_get_heap(vm));

    if (*startData->messageFromParentToChildRef) {
        matteString_t * str = *startData->messageFromParentToChildRef;
        *startData->messageFromParentToChildRef = NULL;
        matte_value_into_string(matte_vm_get_heap(vm), &out, str);
        matte_string_destroy(str);
    }
    return out;
}

static void * matte_thread(void * userData) {
    
    MatteAsyncStartData * startData = userData;
    uint32_t lenBytes;
    *startData->stateRef = MWAS_UNKNOWN;
    uint8_t * src = dump_bytes(startData->from, &lenBytes);
    matteString_t * fromPath = matte_string_create_from_c_str("%s", startData->from);
    if (!src || !lenBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Could not read from file %s", startData->from);        
        *startData->errorStringRef = str;
        *startData->stateRef = MWAS_FAILED;
        
        free(startData);  
        matte_string_destroy(fromPath);
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
        *startData->stateRef = MWAS_FAILED;
        free(src);
        free(startData);          
        matte_string_destroy(fromPath);
        return NULL;
    }



    
    matte_t * m = matte_create();
    matteVM_t * vm = matte_get_vm(m);
    matteHeap_t * heap = matte_vm_get_heap(vm);

    // first compile all and add them 
    int FILEIDS = matte_vm_get_new_file_id(vm, MATTE_VM_STR_CAST(vm, startData->from));

    

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(heap, FILEIDS, outBytes, outByteLen);
    free(outBytes);
    matte_vm_add_stubs(vm, arr);
    matte_array_destroy(arr);


    matte_vm_set_unhandled_callback(
        vm,
        matte_thread_error,
        startData
    );
    
    matte_vm_set_print_callback(vm, on_async_print, NULL);

    *startData->stateRef = MWAS_RUNNING;

    matteValue_t params = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &params);
    matteValue_t key = matte_heap_new_value(heap);
    matte_value_into_string(heap, &key, MATTE_VM_STR_CAST(vm, "input"));
    matteValue_t value = matte_heap_new_value(matte_vm_get_heap(vm));
    matte_value_into_string(heap, &value, startData->input);
    matte_value_object_set(matte_vm_get_heap(vm), params, key, value, 0);

    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::asyncworker_send_message"),   1, matte_asyncworker__send_message, startData);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::asyncworker_check_message"),   0, matte_asyncworker__check_message, startData);

    matteValue_t v = matte_vm_run_fileid(vm, FILEIDS, params, fromPath);
    matte_string_destroy(fromPath);

    

    matteValue_t vStr = matte_value_as_string(heap, v);

    if (v.binID == MATTE_VALUE_TYPE_STRING) {
        *startData->resultRef = matte_string_clone(matte_value_string_get_string_unsafe(heap, v));
        matte_heap_recycle(heap, v);        
    } else {
        if (*startData->stateRef != MWAS_FAILED) {
            *startData->stateRef = MWAS_FAILED;        
            matteString_t * str = matte_string_create();
            matte_string_concat_printf(str, "Could not convert result into string.");        
            *startData->errorStringRef = str;
        }
    }

    matte_destroy(m);
    if (!*startData->errorStringRef)
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
    matteValue_t path  = args[0];
    matteValue_t input = args[1];

    MatteWorkerChildInfo * ch = calloc(1, sizeof(MatteWorkerChildInfo));
    MatteAsyncStartData * init = calloc(1, sizeof(MatteAsyncStartData));
    init->errorStringRef = &ch->errorString;
    init->messageFromChildToParentRef = &ch->messageFromChildToParent;
    init->messageFromParentToChildRef = &ch->messageFromParentToChild;
    matteString_t * expanded = matte_vm_import_expand_path(vm, matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, path)));
    init->from = strdup(matte_string_get_c_str(expanded));
    matte_string_destroy(expanded);
    init->input = matte_string_clone(matte_value_string_get_string_unsafe(heap, input));
    init->stateRef = &(ch->state);
    init->fromVM = vm;
    init->resultRef = &(ch->result);
    ch->workerid = matte_async_next_id();
    
    matte_array_push(asyncSelf.children, ch);
    pthread_create(&(ch->pthreadID), NULL, matte_thread, init);
    
    matteValue_t id = matte_heap_new_value(heap);
    matte_value_into_number(heap, &id, ch->workerid);
    return id;
}


MATTE_EXT_FN(matte_async__state) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    int id = matte_value_as_number(heap, args[0]);
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_heap_new_value(heap);
    }
    
    matteValue_t result = matte_heap_new_value(heap);
    matte_value_into_number(heap, &result, ch->state);
    return result;   
}



MATTE_EXT_FN(matte_async__result) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    int id = matte_value_as_number(heap, args[0]);
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
    int id = matte_value_as_number(heap, args[0]);
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


static void push_messages() {
    int i, n;
    uint32_t len  = matte_array_get_size(asyncSelf.messagesPending),
             nlen = matte_array_get_size(asyncSelf.children);
    for(i = 0; i < len; ++i) {
        MatteWorkerPendingMessage pending = matte_array_at(asyncSelf.messagesPending, MatteWorkerPendingMessage, i);
            
        for(n = 0; n < nlen; ++n) {
            MatteWorkerChildInfo * ch = matte_array_at(asyncSelf.children, MatteWorkerChildInfo *, n);
            if (ch->workerid == pending.id) {
                if (ch->messageFromParentToChild == NULL) {
                    ch->messageFromParentToChild = pending.message;
                    matte_array_remove(asyncSelf.messagesPending, i);
                    len--;
                    i--;
                }
                break;            
            }
        }
    }
}

MATTE_EXT_FN(matte_async__sendmessage) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteWorkerPendingMessage pending;

    pending.id = matte_value_as_number(heap, args[0]);
    pending.message = matte_string_clone(matte_value_string_get_string_unsafe(heap, args[1]));
    matte_array_push(asyncSelf.messagesPending, pending);
    
    push_messages();
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_async__nextmessage) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    push_messages();
    // first update children
    uint32_t i;
    uint32_t len = matte_array_get_size(asyncSelf.children);
    for(i = 0; i < len; ++i) {
        MatteWorkerChildInfo * ch = matte_array_at(asyncSelf.children, MatteWorkerChildInfo *, i);
        if (ch->messageFromChildToParent) {
            // ASYNC
            matteString_t * message = ch->messageFromChildToParent;
            ch->messageFromChildToParent = NULL;
            MatteWorkerPendingMessage pending;
            pending.message = message;
            pending.id = ch->workerid;
            matte_array_push(asyncSelf.messagesReceived, pending);
        }
    }

    matteValue_t v = matte_heap_new_value(heap);
    uint32_t pendingCount = matte_array_get_size(asyncSelf.messagesReceived);
    if (pendingCount) {
        MatteWorkerPendingMessage pending = matte_array_at(asyncSelf.messagesReceived, MatteWorkerPendingMessage, pendingCount-1);
        matte_array_set_size(asyncSelf.messagesReceived, pendingCount-1);

        matteValue_t id = matte_heap_new_value(heap);
        matte_value_into_number(heap, &id, pending.id); 
        matteValue_t message = matte_heap_new_value(heap);
        matte_value_into_string(heap, &message, pending.message); 
        matteValue_t arrSrc[] = {id, message};
        matteArray_t arr = MATTE_ARRAY_CAST(arrSrc, matteValue_t, 2);

        matte_value_into_new_object_array_ref(heap, &v, &arr);
    }
    return v;    
}

static void matte_system__async_cleanup(matteVM_t * vm, void * nu) {
    matte_array_destroy(asyncSelf.messagesReceived);
    matte_array_destroy(asyncSelf.messagesPending);
    matte_array_destroy(asyncSelf.children);
}

static void matte_system__async(matteVM_t * vm) {
    asyncSelf.messagesReceived = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf.messagesPending  = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf.children = matte_array_create(sizeof(MatteWorkerChildInfo*));
    

    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_start"),   2, matte_async__start,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_state"),   1, matte_async__state,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_result"),  1, matte_async__result,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_error"),   1, matte_async__error,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_sendmessage"),   2, matte_async__sendmessage,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_nextmessage"),   0, matte_async__nextmessage,   NULL);

    matte_vm_add_shutdown_callback(vm, matte_system__async_cleanup, NULL);
}
