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


typedef struct {
    HANDLE winthreadID;

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
static DWORD tlsIndex = {};
//__thread MatteAsyncInfo asyncSelf = {};

typedef struct {
    // parent's VM
    matteVM_t * fromVM;

    // import path to the source. New thread frees
    matteString_t * from;
    
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
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);
    uint32_t i;
    uint32_t len = matte_array_get_size(asyncSelf->children);
    for(i = 0; i < len; ++i) {
        if (matte_array_at(asyncSelf->children, MatteWorkerChildInfo *, i)->workerid == id) {
            return matte_array_at(asyncSelf->children, MatteWorkerChildInfo *, i);
        }    
    }
    return NULL;
}

static void matte_thread_compiler_error(const matteString_t * s, uint32_t line, uint32_t ch, void * d) {
    MatteAsyncStartData * startData = (MatteAsyncStartData*)d;
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
    matteStore_t * store = matte_vm_get_store(vm);
    MatteAsyncStartData * startData = (MatteAsyncStartData*)d;
    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t s = matte_value_object_access_string(store, val, MATTE_VM_STR_CAST(vm, "summary"));
        if (matte_value_type(s)) {
            MatteAsyncStartData * startData = (MatteAsyncStartData*)d;
            *startData->errorStringRef = matte_string_create_from_c_str(
                "Unhandled error: \n%s\n", 
                matte_string_get_c_str(matte_value_string_get_string_unsafe(store, s)), 
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
    MatteAsyncStartData * startData = (MatteAsyncStartData*)userData;
    if (!*startData->messageFromChildToParentRef)
         *startData->messageFromChildToParentRef = matte_string_clone(matte_value_string_get_string_unsafe(matte_vm_get_store(vm), args[0]));    

    return matte_store_new_value(matte_vm_get_store(vm));
}

MATTE_EXT_FN(matte_asyncworker__check_message) {
    MatteAsyncStartData * startData = (MatteAsyncStartData*)userData;
    matteValue_t out = matte_store_new_value(matte_vm_get_store(vm));

    if (*startData->messageFromParentToChildRef) {
        matteString_t * str = *startData->messageFromParentToChildRef;
        *startData->messageFromParentToChildRef = NULL;
        matte_value_into_string(matte_vm_get_store(vm), &out, str);
        matte_string_destroy(str);
    }
    return out;
}

static void * matte_thread(void * userData) {
    
    MatteAsyncStartData * startData = (MatteAsyncStartData*)userData;
    uint32_t lenBytes;
    *startData->stateRef = MWAS_UNKNOWN;
    uint8_t * src = (uint8_t*)dump_bytes(matte_string_get_c_str(startData->from), &lenBytes);
    matteString_t * fromPath = matte_string_create_from_c_str("%s", matte_string_get_c_str(startData->from));
    if (!src || !lenBytes) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "Could not read from file %s", matte_string_get_c_str(startData->from));        
        *startData->errorStringRef = str;
        *startData->stateRef = MWAS_FAILED;
        
        matte_deallocate(startData);  
        matte_string_destroy(fromPath);
        return NULL;
    }

    matte_t * m = matte_create();
    matte_set_importer(m, NULL, NULL);
    uint32_t outByteLen;
    uint8_t * outBytes = matte_compiler_run(
        matte_get_syntax_graph(m),
        src,
        lenBytes,
        &outByteLen,
        matte_thread_compiler_error,
        startData
    );
    matte_deallocate(src);
    
    if (!outByteLen || !outBytes) {
        *startData->stateRef = MWAS_FAILED;
        matte_deallocate(startData);          
        matte_string_destroy(fromPath);
        return NULL;
    }



    
    matteVM_t * vm = matte_get_vm(m);
    matteStore_t * store = matte_vm_get_store(vm);

    // first compile all and add them 
    int FILEIDS = matte_vm_get_new_file_id(vm, startData->from);

    

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(store, FILEIDS, outBytes, outByteLen);
    matte_deallocate(outBytes);
    matte_vm_add_stubs(vm, arr);
    matte_array_destroy(arr);


    matte_vm_set_unhandled_callback(
        vm,
        matte_thread_error,
        startData
    );
    
    matte_vm_set_print_callback(vm, on_async_print, NULL);

    *startData->stateRef = MWAS_RUNNING;

    matteValue_t params = matte_store_new_value(store);
    matte_value_into_new_object_ref(store, &params);
    matteValue_t key = matte_store_new_value(store);
    matte_value_into_string(store, &key, MATTE_VM_STR_CAST(vm, "input"));
    matteValue_t value = matte_store_new_value(matte_vm_get_store(vm));
    matte_value_into_string(store, &value, startData->input);
    matte_value_object_set(matte_vm_get_store(vm), params, key, value, 0);

    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::asyncworker_send_message"),   1, matte_asyncworker__send_message, startData);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::asyncworker_check_message"),   0, matte_asyncworker__check_message, startData);

    matteValue_t v = matte_vm_run_fileid(vm, FILEIDS, params);
    matte_string_destroy(fromPath);

    

    matteValue_t vStr = matte_value_as_string(store, v);

    if (matte_value_type(v) == MATTE_VALUE_TYPE_STRING) {
        *startData->resultRef = matte_string_clone(matte_value_string_get_string_unsafe(store, v));
        matte_store_recycle(store, v);        
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
    matte_deallocate(startData);  
    return NULL;
}


static int matte_async_next_id() {
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);

    int i;
    int len = (int)matte_array_get_size(asyncSelf->children);
    
    for(i = 0; i < len; ++i) {
        if (matte_array_at(asyncSelf->children, MatteWorkerChildInfo, i).workerid == asyncSelf.workeridpool) {
            asyncSelf->workeridpool++;
            i = -1;
        }
    }
    
    return asyncSelf->workeridpool++;
}


MATTE_EXT_FN(matte_async__start) {
    matteStore_t * store = matte_vm_get_store(vm);
    // source ensures strings
    matteValue_t path  = args[0];
    matteValue_t input = args[1];

    MatteWorkerChildInfo * ch = (MatteWorkerChildInfo*)matte_allocate(sizeof(MatteWorkerChildInfo));
    MatteAsyncStartData * init = (MatteAsyncStartData*)matte_allocate(sizeof(MatteAsyncStartData));
    init->errorStringRef = &ch->errorString;
    init->messageFromChildToParentRef = &ch->messageFromChildToParent;
    init->messageFromParentToChildRef = &ch->messageFromParentToChild;
    init->from = matte_string_clone(matte_value_string_get_string_unsafe(store, path));
    init->input = matte_string_clone(matte_value_string_get_string_unsafe(store, input));
    init->stateRef = &(ch->state);
    init->fromVM = vm;
    init->resultRef = &(ch->result);
    ch->workerid = matte_async_next_id();
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);
    matte_array_push(asyncSelf->children, ch);
    
    ch->winthreadID = CreateThread(
        0,
        matte_thread,
        init,
        0,
        NULL
    );
    
    matteValue_t id = matte_store_new_value(store);
    matte_value_into_number(store, &id, ch->workerid);
    return id;
}


MATTE_EXT_FN(matte_async__state) {
    matteStore_t * store = matte_vm_get_store(vm);
    int id = matte_value_as_number(store, args[0]);
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_store_new_value(store);
    }
    
    matteValue_t result = matte_store_new_value(store);
    matte_value_into_number(store, &result, ch->state);
    return result;   
}



MATTE_EXT_FN(matte_async__result) {
    matteStore_t * store = matte_vm_get_store(vm);
    int id = matte_value_as_number(store, args[0]);
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_store_new_value(store);
    }

    if (ch->result) {
        matteValue_t v = matte_store_new_value(store);
        matte_value_into_string(store, &v, ch->result);
        return v;
    } else {
        return matte_store_new_value(store);
    }
}


MATTE_EXT_FN(matte_async__error) {
    matteStore_t * store = matte_vm_get_store(vm);
    int id = matte_value_as_number(store, args[0]);
    MatteWorkerChildInfo * ch = find_child(id);
    if (!ch) {
        matte_vm_raise_error_cstring(vm, "No such async primitive ID.");
        return matte_store_new_value(store);
    }

    if (ch->errorString) {
        matteValue_t v = matte_store_new_value(store);
        matte_value_into_string(store, &v, ch->errorString);
        return v;
    } else {
        return matte_store_new_value(store);
    }
}


static void push_messages() {
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);

    int i, n;
    uint32_t len  = matte_array_get_size(asyncSelf->messagesPending),
             nlen = matte_array_get_size(asyncSelf->children);
    for(i = 0; i < len; ++i) {
        MatteWorkerPendingMessage pending = matte_array_at(asyncSelf->messagesPending, MatteWorkerPendingMessage, i);
            
        for(n = 0; n < nlen; ++n) {
            MatteWorkerChildInfo * ch = matte_array_at(asyncSelf->children, MatteWorkerChildInfo *, n);
            if (ch->workerid == pending.id) {
                if (ch->messageFromParentToChild == NULL) {
                    ch->messageFromParentToChild = pending.message;
                    matte_array_remove(asyncSelf->messagesPending, i);
                    len--;
                    i--;
                }
                break;            
            }
        }
    }
}

MATTE_EXT_FN(matte_async__sendmessage) {
    matteStore_t * store = matte_vm_get_store(vm);
    MatteWorkerPendingMessage pending;

    pending.id = matte_value_as_number(store, args[0]);
    pending.message = matte_string_clone(matte_value_string_get_string_unsafe(store, args[1]));
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);
    matte_array_push(asyncSelf->messagesPending, pending);
    
    push_messages();
    return matte_store_new_value(store);    
}

MATTE_EXT_FN(matte_async__nextmessage) {
    matteStore_t * store = matte_vm_get_store(vm);
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);
    push_messages();
    // first update children
    uint32_t i;
    uint32_t len = matte_array_get_size(asyncSelf->children);
    for(i = 0; i < len; ++i) {
        MatteWorkerChildInfo * ch = matte_array_at(asyncSelf->children, MatteWorkerChildInfo *, i);
        if (ch->messageFromChildToParent) {
            // ASYNC
            matteString_t * message = ch->messageFromChildToParent;
            ch->messageFromChildToParent = NULL;
            MatteWorkerPendingMessage pending;
            pending.message = message;
            pending.id = ch->workerid;
            matte_array_push(asyncSelf->messagesReceived, pending);
        }
    }

    matteValue_t v = matte_store_new_value(store);
    uint32_t pendingCount = matte_array_get_size(asyncSelf->messagesReceived);
    if (pendingCount) {
        MatteWorkerPendingMessage pending = matte_array_at(asyncSelf->messagesReceived, MatteWorkerPendingMessage, pendingCount-1);
        matte_array_set_size(asyncSelf->messagesReceived, pendingCount-1);

        matteValue_t id = matte_store_new_value(store);
        matte_value_into_number(store, &id, pending.id); 
        matteValue_t message = matte_store_new_value(store);
        matte_value_into_string(store, &message, pending.message); 
        matteValue_t arrSrc[] = {id, message};
        matteArray_t arr = MATTE_ARRAY_CAST(arrSrc, matteValue_t, 2);

        matte_value_into_new_object_array_ref(store, &v, &arr);
    }
    return v;    
}

static void matte_system__async_cleanup(matteVM_t * vm, void * nu) {
    MatteAsyncInfo * asyncSelf = TlsGetValue(tlsIndex);
    matte_array_destroy(asyncSelf->messagesReceived);
    matte_array_destroy(asyncSelf->messagesPending);
    matte_array_destroy(asyncSelf->children);
}

static void matte_system__async(matteVM_t * vm) {
    tlsIndex = TlsAlloc();
    MatteAsyncInfo * asyncSelf = matte_allocate(sizeof(MatteWorkerChildInfo));
    TlsSetValue(tlsIndex, asyncSelf);
    asyncSelf->messagesReceived = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf->messagesPending  = matte_array_create(sizeof(MatteWorkerPendingMessage));
    asyncSelf->children = matte_array_create(sizeof(MatteWorkerChildInfo*));
    

    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_start"),   2, matte_async__start,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_state"),   1, matte_async__state,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_result"),  1, matte_async__result,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_error"),   1, matte_async__error,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_sendmessage"),   2, matte_async__sendmessage,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::async_nextmessage"),   0, matte_async__nextmessage,   NULL);

    matte_vm_add_shutdown_callback(vm, matte_system__async_cleanup, NULL);
}
