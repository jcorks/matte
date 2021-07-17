#include "matte_vm.h"

struct matteVM_t {
    matteArray_t * stubs;

    // stubIndex[fileid] -> [stubid]
    matteTable_t * stubIndex;

    // topmost: current frame
    matteArray_t * callstack;
    
    // number of stackframes in use.
    uint32_t stacksize;    
    
    
    // operations pending that MUST be executed BEFORE the pc continues.
    // consists of matteBytecodeStubInstruction_t
    matteArray_t * interruptOps;
    
    
    // for all values
    matteHeap_t * heap;

    // queued errors
    matteArray_t * errors;

    // string -> id into externalFunctionIndex;
    matteTable_t * externalFunctions;
    matteArray_t * externalFunctionIndex;
};


typedef struct {
    matteValue_t (*userFunction)(matteVM_t *, matteArray_t * args, void * userData),
    void * userData;
    uint8_t nArgs;
} ExternalFunctionSet_t;

static matteVMStackFrame_t * vm_push_frame(matteVM_t * vm) {
    stackSize++;
    matteVMStackFrame_t * frame;
    if (matte_array_get_size(vm->callstack) < stackSize) {
        matte_array_set_size(vm->callstack, stackSize);
        frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, stackSize-1);

        // initialize
        frame->pc = 0;
        frame->prettyName = matte_string_create();
        frame->context = matte_heap_new_value(vm->heap); // will contain captures
        frame->stub = NULL;
        frame->arguments = matte_array_create(sizeof(matteValue_t));
        frame->locals = matte_array_create(sizeof(matteValue_t));
        frame->valueStack = matte_array_create(sizeof(matteValue_t));
    } else {
        frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, stackSize-1);
        frame->stub = NULL;
        matte_string_clear(frame->prettyName);                
    }
    returm frame;
}


static matteBytecodeStub_t * vm_find_stub(matteVM_t * vm, uint16_t fileid, uint16_t stubid) {
    matteTable_t * subt = matte_table_find_by_uint(vm->subIndex, fileid);
    if (!subt) return NULL;
    return matte_table_find_by_uint(subt, stubid);
}



static matteValue_t vm_execution_loop(matteVM_t * vm) {
    matteVMStackFrame_t * frame = matte_vm_get_stackframe(vm, 0);
    matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);
    matteValue_t output = matte_heap_new_value(vm->heap);

    while(frame->pc < instCount) {
        inst = program[frame->pc++];
        switch(inst->opcode) {
          case MATTE_OPCODE_NOP:
            break;
            
          default: 
            matte_vm_raise_error_string(vm, "Unknown / unhandled opcode.")    
        
        }


        // errors are checked after every instruction.
        // once encountered they are handled immediately.
        // If there is no handler, the error propogates down the callstack.
        if (matte_array_get_size(vm->errors)) {
            matteValue_t v = matte_value_object_access_string(frame->context, MATTE_STR_CAST("errorHandler"));

            // output wont be sent since there was an error. empty will return instead.
            matte_heap_recycle(output);

            if (matte_value_is_callable(v) {
                matteArray_t * cachedErrors = matte_array_clone(vm->errors);
                matte_array_set_size(vm->errors, 0);

                uint32_t i;
                uint32_t len = matte_array_get_size(vm->errors);
                for(i = 0; i < len; ++i) {
                    matteValue_t err = matte_array_at(vm->errors, matteValue_t, i);
                    matte_vm_call(vm, v, MATTE_ARR_CAST(&err, matteValue_t, 1));
                    matte_heap_recycle(err);
                } 
                matte_array_destroy(cachedErrors);
            }

            return matte_heap_new_value(vm->heap);
        }
    }

    return output;
}


matteVM_t * matte_vm_create() {
    matteVM_t * vm = calloc(1, sizeof(matteVM_t));
    vm->stubs = matte_array_create(sizeof(matteBytecodeStub_t*));
    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    vm->externalFunctions = matte_table_create_hash_matte_string();
    vm->externalFunctionList = matte_array_create(sizeof(ExternalFunctionSet_t));
    return vm;
}


matteHeap_t * matte_vm_get_heap(matteVM_t * vm) {return vm->heap;}


matteValue_t matte_vm_call(
    matteVM_t * vm, 
    matteValue_t func, 
    const matteArray_t * args
) {
    if (!matte_value_is_callable(func)) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Error: cannot call non-function value."));
        return matte_heap_new_value(vm->heap);
    }



    if (matte_bytecode_stub_get_fileid(frame->stub) == 0) {
        // fileid 0 is a special fileid that never refers to a real file.
        // this is used to call external c functions. stubID refers to 
        // which index within externalFunction.
        // In this case, a new stackframe is NOT pushed.
        uint16_t external = matte_bytecode_stub_get_stub_if(frame->stub);
        if (external >= matte_array_get_size(vm->externalFunctionIndex)) {
            return matte_heap_new_value(vm->heap);            
        }
        ExternalFunctionSet_t * set = matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, external);

        uint32_t i;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(frame->stub);
        matteArray_t * argsReal = matte_array_create(sizeof(matteValue_t));
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);

            // copy as many args as there are available.
            // All remaining args are empty.
            if (i < lenReal)
                matte_value_into_copy(&v, matte_array_at(args, matteValue_t, i));

            matte_array_push(argsReal, v);
        }


        matteValue_t result = set->userFunction(vm, argsReal, set->userData);



        len = matte_array_get_size(argsReal);
        for(i = 0; i < len; ++i) {
            matte_heap_recycle(matte_array_at(argsReal, matteValue_t, i));
        }
        matte_array_destroy(argsReal);
    } else {
        // no calling context yet
        matteVMStackFrame_t * frame = vm_push_frame(vm);
        frame->context = func;
        frame->stub = matte_value_get_bytecode_stub(func);

        uint32_t i;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(frame->stub);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);

            // copy as many args as there are available.
            // All remaining args are empty.

            if (i < lenReal)
                matte_value_into_copy(&v, matte_array_at(args, matteValue_t, i));

            matte_array_push(frame->arguments, v);
        }

        len = matte_bytecode_stub_get_local_count(frame->stub);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_array_push(frame->locals, v);
        }

        matteValue_t result = vm_execution_loop(vm);


        // cleanup;
        len = matte_array_get_size(frame->arguments);
        for(i = 0; i < len; ++i) {
            matte_heap_recycle(matte_array_at(frame->arguments, matteValue_t, i));
        }
        len = matte_array_get_size(frame->locals);
        for(i = 0; i < len; ++i) {
            matte_heap_recycle(matte_array_at(frame->locals, matteValue_t, i));
        }

        matte_heap_garbage_collect(vm->heap);


        // uh oh... unhandled errors...
        if (vm->stackSize && matte_array_get_size(vm->errors)) {
            #ifdef MATTE_DEBUG
                assert(!"Unhandled error generated!!");
            #endif
        }
        return result;
    } 
}

matteValue_t matte_vm_run_script(
    matteVM_t * vm, 
    uint16_t fileid, 
    const matteArray_t * args
) {
    matteValue_t func = matte_heap_new_value(vm->heap);
    matteBytecodeStub_t * stub = vm_find_stub(vm, fileid, 0);
    if (!stub) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Script has no toplevel context to run."))
        return func;
    }
    matte_value_into_new_function_ref(func, stub);
    matteValue_t result = matte_vm_call(vm, func, args);

    matte_heap_recycle(vm, func);
    return result;
}

void matte_vm_raise_error(matteVM_t * vm, matteValue_t val) {
    matteValue_t b = matte_heap_new_value(vm->heap);
    matte_value_into_copy(&b, val);
    matte_array_push(vm->errors, b);
}

void matte_vm_raise_error_string(matteVM_t * vm, const matteString_t * str) {
    matteValue_t b = matte_heap_new_value(vm->heap);
    matte_value_into_string(&b, str);
    matte_array_push(vm->errors, b);
}


// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i) {
    matteVMStackFrame_t * frames = matte_array_get_data(vm->callstack);
    i = vm->stackSize - 1 - i;
    if (i >= vm->stackSize) { // invalid or overflowed
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested."));
        matteVMStackFrame_t err = {0};
        return err;
    }
    return frames[i];
}

matteValue_t matte_vm_stackframe_get_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID) {
    matteVMStackFrame_t * frames = matte_array_get_data(vm->callstack);
    i = vm->stackSize - 1 - i;
    if (i >= vm->stackSize) { // invalid or overflowed
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested in referrable query."));
        return matte_heap_new_value(vm->heap);
    }

    matteValue_t out = matte_heap_new_value(vm->heap);

    // get context
    if (referrableID == 0) {
        matte_value_into_copy(&out, frames[i].context);
        return out;
    }

    // next, arguments
    uint32_t offset = 1;
    uint32_t max  = offset+matte_array_get_size(frames[i].arguments);
    if (referrableID < max) {
        matte_value_into_copy(&out, matte_array_at(frames[i].arguments, matteValue_t, referrableID - offset);
        return out;        
    }

    // next, locals
    uint32_t offset = max;
    uint32_t max  = offset+matte_array_get_size(frames[i].locals);
    if (referrableID < max) {
        matte_value_into_copy(&out, matte_array_at(frames[i].locals, matteValue_t, referrableID - offset);
        return out;        
    }

    // finally
    const matteArray_t * captured = matte_value_get_captured_values(frames[i].context);
    uint32_t offset = max;
    uint32_t max  = offset+matte_array_get_size(captured);
    if (referrableID < max) {
        matte_value_into_copy(&out, matte_array_at(captured, matteValue_t, referrableID - offset);
        return out;        
    }

    // bad referrable
    matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid referrable"));
    return out;    
}



void matte_vm_set_external_function(
    matteVM_t * vm, 
    matteString_t * identifier
    uint8_t nArgs,
    matteValue_t (*)(matteVM_t *, matteArray_t * args, void * userData),
    void * userData
) {
    if (!userFunction) return 0;



    ExternalFunctionSet_t * set;    
    uint16_t * id = matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        id = malloc(sizeof(uint16_t));
        *id = matte_array_get_size(vm->externalFunctionIndex);
        matte_array_set_size(vm->externalFunctionIndex, *id+1);
    }
    set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, *id);
    set->userFunction = userFunction;
    set->userData = userData;
    set->nArgs = nArgs;
}

// Gets a new function object that, when called, calls the registered 
// C function.
matteValue_t matte_vm_get_external_function_as_value(
    matteVM_t * vm,
    matteString_t * identifier
) {
    uint16_t * id = matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        return matte_heap_new_value(vm->heap);
    }
    // to do this, we have a special stub ROM that 
    // will always run an external function
    typedef struct {
        uint16_t filestub;
        uint16_t stubid;
    } fakestub;

    fakestub f;
    f.filestub = 0;
    f.stubid = *id;

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(&fakestub, sizeof(uint32_t));
    if (matte_array_get_size(arr) == 0) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("External function conversion failed (truncated stub was denied???)"));
    }
    matteBytecodeStub_t * stub = matte_array_at(arr, matteBytecodeStub_t *, 0);

    matteValue_t func;
    matte_value_into_new_function_ref(&func, stub);
    matte_bytecode_stub_destroy(stub);
    matte_array_destroy(arr);

    return func;
}



