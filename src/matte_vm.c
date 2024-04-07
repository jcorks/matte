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
#include "matte_vm.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_store.h"
#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_opcode.h"
#include "matte.h"
#include "matte_compiler.h"
#include "matte.h"
#include "./rom/native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


#define matte_string_temp_max_calls 128
struct matteVM_t {
    matte_t * matte;

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
    matteStore_t * store;

    // queued thrown object. if none, error.binID == 0
    matteValue_t catchable;
    

    // string -> id into externalFunctionIndex;
    matteTable_t * externalFunctions;
    matteArray_t * externalFunctionIndex;

    // symbolic stubs for all the ext calls.
    // full of matteBytecodeStub_t *
    matteArray_t * extStubs;
    matteArray_t * extFuncs;


    void(*debug)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t, void *);
    void * debugData;

    void(*unhandled)(matteVM_t *, uint32_t file, int lineNumber, matteValue_t, void *);
    void * unhandledData;

   
    // last line (for debug line change)  
    uint32_t lastLine;

    // stackframe index for debug calls.
    int namedRefIndex;


    // table of all run script results in the VM.
    matteTable_t * imported;
    matteTable_t * importName2ID;
    matteTable_t * id2importName;

    // import for scripts
    matteImportFunction_t importFunction;
    void * importFunctionData;
    uint32_t nextID;

    int pendingCatchable;
    int pendingCatchableIsError;
    int errorLastLine;
    int userGCFreeze;
    uint32_t errorLastFile;


    void (*userPrint)(
        matteVM_t * vm,
        const matteString_t *,
        void * userData
    );
    void * userPrintData;

    matteString_t * string_tempVals[matte_string_temp_max_calls];
    int string_tempIter;

    // for parts of the implementation that request it, 
    // if these are set, the next pushed stackframe will contain 
    // these as the restart condition dataset.
    int (*pendingRestartCondition)(matteVM_t *,matteVMStackFrame_t *, matteValue_t, void *);
    void * pendingRestartConditionData;


    // called before anything else in vm_destroy
    matteArray_t * cleanupFunctionSets;
    

    matteValue_t specialString_parameters;
    matteValue_t specialString_from;
    matteValue_t specialString_value;
    matteValue_t specialString_message;
    matteValue_t specialString_previous;
    matteValue_t specialString_base;
    
    matteValue_t specialString_onerror;
    matteValue_t specialString_onsend;

};


typedef struct {
    matteValue_t value;
    uint32_t aux;
} matteValue_Extended_t;


typedef struct {
    void (*fn)(matteVM_t *, void *);
    void * data;
} MatteCleanupFunctionSet;


#ifdef MATTE_DEBUG

void matte_vm_find_in_stack(matteVM_t * vm, uint32_t id) {
    uint32_t i;
    for(i = 0; i < vm->stacksize; ++i) {
        matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t *, i);
        
        uint32_t n = 0;
        for(n = 0; n < matte_array_get_size(frame->valueStack); ++n) {
            matteValue_t v = matte_array_at(frame->valueStack, matteValue_Extended_t, n).value;
            if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
                printf("@ stackframe %d, valuestack %d\n", i, n);
            }
        }
    }
}

#endif


matteValue_t * matte_vm_stackframe_get_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID);
void matte_vm_stackframe_set_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID, matteValue_t val);

typedef struct {
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, const matteValue_t * args, void * userData);
    void * userData;
    uint8_t nArgs;
} ExternalFunctionSet_t;




matteValue_t matte_vm_call_full(
    matteVM_t * vm, 
    matteValue_t func, 
    matteValue_t privateBinding,
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
);



// Function call with just one argument that is splayed to 
// fill the calling functions arguments as best as possible.
// The missing arguments are not matched, and any extra 
// object names are ignored.
static matteValue_t matte_vm_vararg_call(
    matteVM_t * vm, 
    matteValue_t func, 
    matteValue_t callingContext,
    matteValue_t args,
    matteValue_t dynBind
) {
    if (matte_value_type(args) != MATTE_VALUE_TYPE_OBJECT) {
        matteString_t * err = matte_string_create_from_c_str("Vararg calls MUST provide an expression that results in an Object");
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_store_new_value(vm->store);
    }
    if (!matte_value_is_function(func)) {    
        matteString_t * err = matte_string_create_from_c_str("Cannot use a vararg call on something that isnt a function.");
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_store_new_value(vm->store);
    }
    
    matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, func);
    matteArray_t * argVals = matte_array_create(sizeof(matteValue_t));
    matteArray_t * argNames = matte_array_create(sizeof(matteValue_t));
    matteValue_t dynBindName = matte_store_get_dynamic_bind_token(vm->store);

    if (matte_value_type(dynBind)) {
        matte_array_push(argVals, dynBind);
        matte_array_push(argNames, dynBindName);
    }


    uint32_t i;
    uint32_t len;

    if (matte_bytecode_stub_is_vararg(stub)) {
        matteValue_t keys = matte_value_object_keys(vm->store, args);
        uint32_t len = matte_value_object_get_number_key_count(vm->store, keys);
        for(i = 0; i < len; ++i) {
            matteValue_t key = matte_value_object_access_index(vm->store, keys, i);
            if (matte_value_type(key) != MATTE_VALUE_TYPE_STRING) continue;
            
            if (key.value.id == dynBindName.value.id) {
                continue;
            }
            
            matte_array_push(argNames, key);
            matteValue_t arg = matte_value_object_access(
                vm->store, 
                args, 
                key, 
                0
            );
            matte_array_push(argVals, arg);
        }        
    } else {
        len = matte_bytecode_stub_arg_count(stub);
        for(i = 0; i < len; ++i) {
            matteValue_t name = matte_bytecode_stub_get_arg_name(stub, i);            
            if (name.value.id == dynBindName.value.id) {
                continue;
            }

            matteValue_t arg = matte_value_object_access(
                vm->store, 
                args, 
                name, 
                0
            );
            
            matte_array_push(argVals, arg);
            matte_array_push(argNames, name);
        }
    }    

    matteValue_t out = matte_vm_call_full(vm, func, callingContext, argVals, argNames, NULL);
    
    len = matte_array_get_size(argVals);
    for(i = 0; i < len; ++i) {
        matte_store_recycle(
            vm->store,
            matte_array_at(argVals, matteValue_t, i)
        );
        
        // todo: recycle vararg keys?
    }
    matte_array_destroy(argVals);
    matte_array_destroy(argNames);
    
    return out;
}




const matteString_t * matte_vm_cstring_to_string_temporary(matteVM_t * vm, const char * s) {
    if (vm->string_tempIter >= matte_string_temp_max_calls) 
        vm->string_tempIter = 0;
    matteString_t * t = vm->string_tempVals[vm->string_tempIter++];
    if (!t) {
        t = matte_string_create();
        vm->string_tempVals[vm->string_tempIter-1] = t;
    }   
    matte_string_clear(t);
    matte_string_concat_printf(t, "%s", s);
    return t;
}


void matte_vm_set_import(
    matteVM_t * vm,
    matteImportFunction_t import,
    void * userData
) {
    vm->importFunction = import;
    vm->importFunctionData = userData;
}



// todo: how to we implement this in a way that we dont "waste"
// IDs by failed /excessive calls to this when the IDs arent used?
uint32_t matte_vm_get_new_file_id(matteVM_t * vm, const matteString_t * name) {
    uint32_t fileid = vm->nextID++; 

    uint32_t * fileidPtr = (uint32_t*)matte_allocate(sizeof(uint32_t));
    *fileidPtr = fileid;

    matte_table_insert(vm->importName2ID, name, fileidPtr);   
    matte_table_insert_by_uint(vm->id2importName, fileid, matte_string_clone(name));

    return fileid;
}


uint32_t matte_vm_get_file_id_by_name(matteVM_t * vm, const matteString_t * name) {
    uint32_t * p = (uint32_t*)matte_table_find(vm->importName2ID, name);   
    if (!p) return 0xffffffff;
    return *p;
}



static matteVMStackFrame_t * vm_push_frame(matteVM_t * vm) {
    vm->stacksize++;
    matteVMStackFrame_t * frame;
    if (matte_array_get_size(vm->callstack) < vm->stacksize+1) { // push an extra for stackframe look-ahead
        while(matte_array_get_size(vm->callstack) < vm->stacksize+1) {
            frame = (matteVMStackFrame_t*)matte_allocate(sizeof(matteVMStackFrame_t));
            // initialize
            frame->referrables = matte_array_create(sizeof(matteValue_t));
            frame->pc = 0;
            frame->prettyName = matte_string_create();
            frame->context = matte_store_new_value(vm->store); // will contain captures
            frame->stub = NULL;
            frame->valueStack = matte_array_create(sizeof(matteValue_Extended_t));

            matte_array_push(vm->callstack, frame);
        }
        frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);
    } else {
        frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);
        frame->stub = NULL;
        matte_string_clear(frame->prettyName);     
        matte_array_set_size(frame->valueStack, 0);
        frame->pc = 0;
           
    }

    if (vm->pendingRestartCondition) {
        frame->restartCondition     = vm->pendingRestartCondition;
        frame->restartConditionData = vm->pendingRestartConditionData;
        vm->pendingRestartCondition = NULL;
        vm->pendingRestartConditionData = NULL;
    } else {
        frame->restartCondition = NULL;
        frame->restartConditionData = NULL;
    }
    return frame;
}


static void vm_pop_frame(matteVM_t * vm) {
    if (vm->stacksize == 0) return; // error??    
    vm->stacksize--;
}


static matteBytecodeStub_t * vm_find_stub(matteVM_t * vm, uint32_t fileid, uint32_t stubid) {
    matteTable_t * subt = (matteTable_t*)matte_table_find_by_uint(vm->stubIndex, fileid);
    if (!subt) return NULL;
    return (matteBytecodeStub_t*)matte_table_find_by_uint(subt, stubid);
}


static matteValue_t vm_listen(matteVM_t * vm, matteValue_t v, matteValue_t respObject) {
    if (!matte_value_is_callable(vm->store, v)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Listen expressions require that the listened-to expression is a function. "));
        return matte_store_new_value(vm->store);
    }

    if (matte_value_type(respObject) != MATTE_VALUE_TYPE_EMPTY && matte_value_type(respObject) != MATTE_VALUE_TYPE_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Listen requires that the response expression is an object."));
        return matte_store_new_value(vm->store);    
    }
    
    matte_value_object_push_lock(vm->store, respObject);
    
    matteValue_t * onSend = NULL;
    matteValue_t * onError = NULL;    
    matteValue_t onErrorVal = {};
    matteValue_t onSendVal = {};
    
    if (matte_value_type(respObject) != MATTE_VALUE_TYPE_EMPTY) {
        onSend  = matte_value_object_access_direct(vm->store, respObject, vm->specialString_onsend, 0);
        if (onSend && matte_value_type(*onSend) != MATTE_VALUE_TYPE_EMPTY && !matte_value_is_callable(vm->store, *onSend)) {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Listen requires that the response object's 'onSend' attribute be a Function."));
            matte_value_object_pop_lock(vm->store, respObject);
            return matte_store_new_value(vm->store);    
        }
        if (onSend) {
            onSendVal = *onSend;
        }
        onError = matte_value_object_access_direct(vm->store, respObject, vm->specialString_onerror, 0);
        if (onError && matte_value_type(*onError) != MATTE_VALUE_TYPE_EMPTY && !matte_value_is_callable(vm->store, *onError)) {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Listen requires that the response object's 'onError' attribute be a Function."));
            matte_value_object_pop_lock(vm->store, respObject);
            return matte_store_new_value(vm->store);    
        }
        if (onError) {
            onErrorVal = *onError;
        }
    }
    matte_value_object_push_lock(vm->store, v);
    matteValue_t out = matte_vm_call(vm, v, matte_array_empty(), matte_array_empty(), MATTE_VM_STR_CAST(vm, "listen"));
    matte_value_object_pop_lock(vm->store, v);
    if (vm->pendingCatchable) {

        matte_store_recycle(vm->store, out);            
        matteValue_t catchable = vm->catchable;
        vm->catchable.binIDreserved = 0;
        vm->pendingCatchable = 0;


        matteArray_t arr = MATTE_ARRAY_CAST(&catchable, matteValue_t, 1);
        matteArray_t arrNames = MATTE_ARRAY_CAST(&vm->specialString_message, matteValue_t, 1);


        // if the catchable exists, we either 
        // 1) return this return value 
        // 2) run a response function and return its result
        if (!vm->pendingCatchableIsError && onSend) {
            out = matte_vm_call(vm, onSendVal, &arr, &arrNames, MATTE_VM_STR_CAST(vm, "listen response (message)"));
            matte_value_object_pop_lock(vm->store, respObject);
            matte_value_object_pop_lock(vm->store, catchable);
            matte_store_recycle(vm->store, catchable);
            return out;
        } if (vm->pendingCatchableIsError && onError) {
            // the error is caught. Undo the error flag 
            vm->pendingCatchableIsError = 0;
            out = matte_vm_call(vm, onErrorVal, &arr, &arrNames, MATTE_VM_STR_CAST(vm, "listen response (error)"));
            matte_value_object_pop_lock(vm->store, respObject);
            matte_value_object_pop_lock(vm->store, catchable);
            matte_store_recycle(vm->store, catchable);
            return out;
        } else {
            matte_value_object_pop_lock(vm->store, respObject);
            if (vm->pendingCatchableIsError) {
                // The error is uncaught, and must be handled properly before continuing.
                vm->catchable = catchable;
                vm->pendingCatchable = 1;
            } else {
                // OK, popped but not recycled. 
                matte_value_object_pop_lock(vm->store, catchable);           
                return catchable;
            }
        }
        return matte_store_new_value(vm->store);
        
    } else {
        // OK, already new from vm_call
        matte_value_object_pop_lock(vm->store, respObject);           
        return out;        
    }
}



#include "MATTE_OPERATORS"
#include "MATTE_EXT_CALLS"







static matteValue_t vm_operator_2(matteVM_t * vm, matteOperator_t op, matteValue_t a, matteValue_t b) {
    switch(op) {
      case MATTE_OPERATOR_ADD:        return vm_operator__add         (vm, a, b);
      case MATTE_OPERATOR_SUB:        return vm_operator__sub         (vm, a, b);
      case MATTE_OPERATOR_DIV:        return vm_operator__div         (vm, a, b);
      case MATTE_OPERATOR_MULT:       return vm_operator__mult        (vm, a, b);
      case MATTE_OPERATOR_BITWISE_OR: return vm_operator__bitwise_or  (vm, a, b);
      case MATTE_OPERATOR_OR:         return vm_operator__or          (vm, a, b);
      case MATTE_OPERATOR_BITWISE_AND:return vm_operator__bitwise_and (vm, a, b);
      case MATTE_OPERATOR_AND:        return vm_operator__and         (vm, a, b);
      case MATTE_OPERATOR_SHIFT_LEFT: return vm_operator__shift_left  (vm, a, b);
      case MATTE_OPERATOR_SHIFT_RIGHT:return vm_operator__shift_right (vm, a, b);
      case MATTE_OPERATOR_POW:        return vm_operator__pow         (vm, a, b);
      case MATTE_OPERATOR_EQ:         return vm_operator__eq          (vm, a, b);
      case MATTE_OPERATOR_POINT:      return vm_operator__overload_only_2(vm, "->", a, b);
      case MATTE_OPERATOR_TERNARY:    return vm_operator__overload_only_2(vm, "?", a, b);
      case MATTE_OPERATOR_LESS:       return vm_operator__less        (vm, a, b);
      case MATTE_OPERATOR_GREATER:    return vm_operator__greater     (vm, a, b);
      case MATTE_OPERATOR_LESSEQ:     return vm_operator__lesseq      (vm, a, b);
      case MATTE_OPERATOR_GREATEREQ:  return vm_operator__greatereq   (vm, a, b);
      case MATTE_OPERATOR_TRANSFORM:  return vm_operator__overload_only_2(vm, "<>", a, b);
      case MATTE_OPERATOR_NOTEQ:      return vm_operator__noteq       (vm, a, b);
      case MATTE_OPERATOR_MODULO:     return vm_operator__modulo      (vm, a, b);
      case MATTE_OPERATOR_CARET:      return vm_operator__caret       (vm, a, b);
      case MATTE_OPERATOR_TYPESPEC:   return vm_operator__typespec    (vm, a, b);

      default:
        matte_vm_raise_error_cstring(vm, "unhandled OPR operator");                        
        return matte_store_new_value(vm->store);
    }
}


static matteValue_t vm_operator_1(matteVM_t * vm, matteOperator_t op, matteValue_t a) {
    switch(op) {
      case MATTE_OPERATOR_NOT:         return vm_operator__not(vm, a);
      case MATTE_OPERATOR_NEGATE:      return vm_operator__negate(vm, a);
      case MATTE_OPERATOR_BITWISE_NOT: return vm_operator__bitwise_not(vm, a);
      case MATTE_OPERATOR_POUND:       return vm_operator__overload_only_1(vm, "#", a);

      default:
        matte_vm_raise_error_cstring(vm, "unhandled OPR operator");                        
        return matte_store_new_value(vm->store);
    }
}




/*
static const char * opcode_to_str(int oc) {
    switch(oc) {
      case MATTE_OPCODE_NOP: return "NOP";
      case MATTE_OPCODE_PRF: return "PRF";
      case MATTE_OPCODE_NEM: return "NEM";
      case MATTE_OPCODE_NNM: return "NNM";
      case MATTE_OPCODE_NBL: return "NBL";
      case MATTE_OPCODE_NST: return "NST";
      case MATTE_OPCODE_NOB: return "NOB";
      case MATTE_OPCODE_NFN: return "NFN";
      case MATTE_OPCODE_NAR: return "NAR";
      case MATTE_OPCODE_NSO: return "NSO";
        
      case MATTE_OPCODE_CAL: return "CAL";
      case MATTE_OPCODE_ARF: return "ARF";
      case MATTE_OPCODE_OSN: return "OSN";
      case MATTE_OPCODE_OLK: return "OLK";
      case MATTE_OPCODE_OPR: return "OPR";
      case MATTE_OPCODE_EXT: return "EXT";
      case MATTE_OPCODE_POP: return "POP";
      case MATTE_OPCODE_CPY: return "CPY";
      case MATTE_OPCODE_RET: return "RET";
      case MATTE_OPCODE_SKP: return "SKP";
      case MATTE_OPCODE_ASP: return "ASP";
      case MATTE_OPCODE_PNR: return "PNR";
      case MATTE_OPCODE_SFS: return "SFS";
      case MATTE_OPCODE_SCA: return "SCA";
      case MATTE_OPCODE_SCO: return "SCO";

      default:
        return "???";
    }
}
*/

#define STACK_SIZE() matte_array_get_size(frame->valueStack)
#define STACK_POP() matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1).value; matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_POP_NORET() matte_store_recycle(vm->store, matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1).value); matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_PEEK(__n__) (matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1-(__n__))).value
#define STACK_PUSH(__v__) ve_.value = __v__; matte_array_push(frame->valueStack, ve_);

#define STACK_PEEK_EXTENDED(__n__) (matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1-(__n__)))
#define STACK_PUSH_EXTENDED(__VE__) (matte_array_push(frame->valueStack, __VE__));




#define CONTROL_CODE_VALUE_TYPE__DYNAMIC_BINDING 0xff

static int vm_execution_loop__stack_depth = 0;
#define VM_EXECUTABLE_LOOP_STACK_DEPTH_LIMIT 1024
#define VM_EXECUTABLE_LOOP_CURRENT_LINE (matte_bytecode_stub_get_starting_line(frame->stub) + inst->info.lineOffset)
static matteValue_t vm_execution_loop(matteVM_t * vm) {
    vm_execution_loop__stack_depth ++;
    
    if (vm_execution_loop__stack_depth > VM_EXECUTABLE_LOOP_STACK_DEPTH_LIMIT) {
        matte_vm_raise_error_cstring(vm, "Stack call limit reached. (Likely infinite recursion)");
        return matte_store_new_value(vm->store);
    }
    matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);
    #ifdef MATTE_DEBUG__VM
        const matteString_t * str = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub));
    #endif
    const matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    uint32_t sfscount = 0;
    matteValue_Extended_t ve_ = {};
    const matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);

  RELOOP:
    while(frame->pc < instCount) {
        inst = program+frame->pc++;
        

        
        // TODO: optimize out
        #ifdef MATTE_DEBUG__VM
            if (matte_array_get_size(frame->valueStack))
                matte_value_print(vm->store, matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1).value);
            printf("from %s, line %d, CALLSTACK%6d PC%6d, OPCODE %s, Stacklen: %10d\n", str ? matte_string_get_c_str(str) : "???", VM_EXECUTABLE_LOOP_CURRENT_LINE, vm->stacksize, frame->pc, opcode_to_str(inst->info.opcode), matte_array_get_size(frame->valueStack));
            fflush(stdout);
        #endif
        if (vm->debug) {
            if (vm->lastLine != VM_EXECUTABLE_LOOP_CURRENT_LINE) {
                matteValue_t db = matte_store_new_value(vm->store);
                vm->debug(vm, MATTE_VM_DEBUG_EVENT__LINE_CHANGE, matte_bytecode_stub_get_file_id(frame->stub), VM_EXECUTABLE_LOOP_CURRENT_LINE, db, vm->debugData);
                vm->lastLine = VM_EXECUTABLE_LOOP_CURRENT_LINE;
                matte_store_recycle(vm->store, db);
            }
        }


        switch(inst->info.opcode) {
          case MATTE_OPCODE_NOP:
            break;
            
          case MATTE_OPCODE_LST: {
            matteValue_t v = vm_listen(vm, STACK_PEEK(1), STACK_PEEK(0));
            STACK_POP_NORET();
            STACK_POP_NORET();
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_PIP: {
            if (matte_value_type(frame->privateBinding) == MATTE_VALUE_TYPE_EMPTY) {
                matte_vm_raise_error_cstring(vm, "The private interface binding is only available for functions that are called directly from an interface.");            
            }
            STACK_PUSH(frame->privateBinding);
            break;
          }
            
          case MATTE_OPCODE_PRF: {
            uint32_t referrable = (uint32_t)inst->data;
            matteValue_t * v = matte_vm_stackframe_get_referrable(vm, 0, referrable);
            if (v) {
                matteValue_t copy = matte_store_new_value(vm->store);
                matte_value_into_copy(vm->store, &copy, *v);
                STACK_PUSH(copy);
            } else {
                matte_vm_raise_error_cstring(vm, "VM Error: Tried to push non-existant referrable.");
                
            }
            break;
          }
          
          case MATTE_OPCODE_PNR: {
            uint32_t referrableStrID = (double) inst->data;
            matteValue_t v = matte_bytecode_stub_get_string(frame->stub, referrableStrID);
            if (!matte_value_type(v)) {
                matte_vm_raise_error_cstring(vm, "VM Error: No such bytecode stub string.");
            } else {
                matteVMStackFrame_t f = matte_vm_get_stackframe(vm, vm->namedRefIndex+1);
                if (matte_value_type(f.context)) {
                    matteValue_t v0 = matte_value_frame_get_named_referrable(vm->store, 
                        &f, 
                        v
                    ); 
                    STACK_PUSH(v0);
                }
            }
            break;
          }

          case MATTE_OPCODE_NEM: {
            matteValue_t v = matte_store_new_value(vm->store);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_NNM: {
            matteValue_t v = {};
            matte_value_into_number(vm->store, &v, inst->data);
            STACK_PUSH(v);

            break;
          }
          case MATTE_OPCODE_NBL: {
            matteValue_t v = matte_store_new_value(vm->store);
            matte_value_into_boolean(vm->store, &v, inst->data!=0.0);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_NST: {
            uint32_t stringID = inst->data;

            // NO XFER
            matteValue_t v = matte_bytecode_stub_get_string(frame->stub, stringID);
            if (!matte_value_type(v)) {
                matte_vm_raise_error_cstring(vm, "NST opcode refers to non-existent string (corrupt bytecode?)");
                break;
            }
            matteValue_t out = matte_store_new_value(vm->store);
            matte_value_into_copy(vm->store, &out, v);
            STACK_PUSH(out);
            break;
          }
          case MATTE_OPCODE_NOB: {
            matteValue_t v = matte_store_new_value(vm->store);
            matte_value_into_new_object_ref(vm->store, &v);
            #ifdef MATTE_DEBUG__STORE
                matte_store_track_neutral(vm->store, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), VM_EXECUTABLE_LOOP_CURRENT_LINE);
            #endif
            STACK_PUSH(v);
            break;
          }
          

          case MATTE_OPCODE_NEF: {
            matteValue_t v = matte_store_empty_function(vm->store);
            STACK_PUSH(v);
            break;
          }
                    
          case MATTE_OPCODE_SFS:
            sfscount = (uint32_t)inst->data;

            if (frame->pc >= instCount) {
                break;
            }
            inst = program+frame->pc++;

            // FALLTHROUGH PURPOSEFULLY          
          
          case MATTE_OPCODE_NFN: {
            uint32_t ids[2];
            ids[0] = inst->funcData.nfnFileID;
            ids[1] = inst->funcData.stubID;

            matteBytecodeStub_t * stub = vm_find_stub(vm, ids[0], ids[1]);

            if (stub == NULL) {
                matte_vm_raise_error_cstring(vm, "NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)");
                break;
            }

            if (sfscount) {
                matteValue_t v = matte_store_new_value(vm->store);
                uint32_t i;
                if (STACK_SIZE() < sfscount) {
                    matte_vm_raise_error_cstring(vm, "VM internal error: too few values on stack to service SFS opcode!");
                    break;
                }
                matteValue_t * vals = (matteValue_t*)matte_allocate(sfscount*sizeof(matteValue_t));
                // reverse order since on the stack is [retval] [arg n-1] [arg n-2]...
                for(i = 0; i < sfscount; ++i) {
                    vals[sfscount - i - 1] = STACK_PEEK(i);
                }

                // xfer ownership of type values
                matteArray_t arr = MATTE_ARRAY_CAST(vals, matteValue_t, sfscount);
                matte_value_into_new_typed_function_ref(vm->store, &v, stub, &arr);
                #ifdef MATTE_DEBUG__STORE
                    matte_store_track_neutral(vm->store, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), VM_EXECUTABLE_LOOP_CURRENT_LINE);
                #endif

                matte_deallocate(vals);

                for(i = 0; i < sfscount; ++i) {
                    STACK_POP_NORET();
                }
                sfscount = 0;
                STACK_PUSH(v);
                
            } else {
                matteValue_t v = matte_store_new_value(vm->store);
                matte_value_into_new_function_ref(vm->store, &v, stub);
                #ifdef MATTE_DEBUG__STORE
                    matte_store_track_neutral(vm->store, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), VM_EXECUTABLE_LOOP_CURRENT_LINE);
                #endif

                STACK_PUSH(v);
            }
            break;
          }

          case MATTE_OPCODE_CAA: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: missing object - value pair for constructor push");    
                break;
            }            
            
            matteValue_t obj = STACK_PEEK(1);
            matte_value_object_push(
                vm->store,
                obj,
                STACK_PEEK(0)
            );

            STACK_POP_NORET();            
            break;
            
          }
          case MATTE_OPCODE_CAS: {
            if (STACK_SIZE() < 3) {
                matte_vm_raise_error_cstring(vm, "VM error: missing object - value pair for constructor push");    
                break;
            }            
            
            matteValue_t obj = STACK_PEEK(2);
            matte_value_object_set(
                vm->store,
                obj,
                STACK_PEEK(1),
                STACK_PEEK(0),
                1
            );

            STACK_POP_NORET();            
            STACK_POP_NORET();            
            break;
            
          }          
          case MATTE_OPCODE_SPA: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");    
                break;
            }
             
            matteValue_t p = STACK_PEEK(0);
            matteValue_t target = STACK_PEEK(1);
                       
            uint32_t len = matte_value_object_get_number_key_count(vm->store, p);
            uint32_t i;
            uint32_t keylen = matte_value_object_get_number_key_count(vm->store, target);
            for(i = 0; i < len; ++i) {
                matte_value_object_insert(
                    vm->store,
                    target, 
                    keylen++,
                    matte_value_object_access_index(vm->store, p, i)
                );
            }
            
            STACK_POP();
            matte_store_recycle(vm->store, p);
            break;            
          }
  

          case MATTE_OPCODE_SPO: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");    
                break;
            }
             
            matteValue_t p = STACK_PEEK(0);
            matteValue_t keys = matte_value_object_keys(vm->store, p);
            matte_value_object_push_lock(vm->store, keys);
            matteValue_t vals = matte_value_object_values(vm->store, p);
            matte_value_object_push_lock(vm->store, vals);

            matteValue_t target = STACK_PEEK(1);
                       
            uint32_t len = matte_value_object_get_number_key_count(vm->store, keys);
            uint32_t i;
            matteValue_t item;            
            for(i = 0; i < len; ++i) {
                matte_value_object_set(
                    vm->store,
                    target,
                    matte_value_object_access_index(vm->store, keys, i),
                    matte_value_object_access_index(vm->store, vals, i),
                    1
                );
            }
            
            matte_value_object_pop_lock(vm->store, keys);
            matte_value_object_pop_lock(vm->store, vals);
            STACK_POP();
            matte_store_recycle(vm->store, p);
            break;            
          }
          
          
          case MATTE_OPCODE_PTO: {
            uint32_t typecode = (uint32_t)inst->data;
            matteValue_t v;
            
            switch(typecode) {
              case 0: v = *matte_store_get_empty_type(vm->store); break;           
              case 1: v = *matte_store_get_boolean_type(vm->store); break;           
              case 2: v = *matte_store_get_number_type(vm->store); break;           
              case 3: v = *matte_store_get_string_type(vm->store); break;           
              case 4: v = *matte_store_get_object_type(vm->store); break;           
              case 5: v = *matte_store_get_function_type(vm->store); break;           
              case 6: v = *matte_store_get_type_type(vm->store); break;           
              case 7: v = *matte_store_get_any_type(vm->store); break;           
              case 8: v = *matte_store_get_nullable_type(vm->store); break;           
                
            }
            STACK_PUSH(v);
            break;
          }
          
          case MATTE_OPCODE_CLV: {

            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for a vararg call, but insufficient arguments on the stack.");    
                break;
            }

            matteValue_Extended_t function = STACK_PEEK_EXTENDED(1);
            matteValue_t result;
            matteValue_t dynBind = {};
            matteValue_t privateBinding = {};

            if (function.aux != 0) {
                matteValue_t m = {};
                m.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
                m.value.id = function.aux;

                matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, function.value);
                if (stub && matte_bytecode_stub_is_dynamic_bind(stub)) {                    
                    dynBind = m;
                }
                
                if (stub && matte_value_object_get_is_interface_unsafe(vm->store, m)) {
                    privateBinding = matte_value_object_get_interface_private_binding_unsafe(vm->store, m);
                }

                
            }
            
            
            
            result = matte_vm_vararg_call(vm, STACK_PEEK(1), privateBinding, STACK_PEEK(0), dynBind);



            STACK_POP_NORET(); // arg
            STACK_POP_NORET(); // fn
            STACK_PUSH(result);
            break;            
          }
  
          case MATTE_OPCODE_CAL: {

            if (STACK_SIZE() < 1) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");    
                break;
            }
            
            


            matteArray_t * args = matte_array_create(sizeof(matteValue_t));
            matteArray_t * argnames = matte_array_create(sizeof(matteValue_t));

            uint32_t i = 0;
            uint32_t stackSize = STACK_SIZE();
            if (stackSize > 2) {
                while(i < stackSize-1) {
                    matteValue_t key = STACK_PEEK(i);
                    matteValue_t value = STACK_PEEK(i+1);
                
                
                    if (matte_value_type(key) == MATTE_VALUE_TYPE_STRING) {
                        matte_array_push(argnames, key);
                        matte_array_push(args, value);                
                    } else {
                        break;
                    }

                    i += 2;
                }
            }
            uint32_t argcount = matte_array_get_size(args);
            /*
            if (i == stackSize) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");    
                matte_array_destroy(args);            
                matte_array_destroy(argnames);            
                break;
            }*/

            matteValue_Extended_t function = STACK_PEEK_EXTENDED(i);
            matteValue_t privateBinding = {};
            // TODO: we need to preserve the dynamic binding to guarantee its always accessible.
            // Right now we just assume it is, and in 99% of cases it will be, but it is 
            // trivial to come up with a case where it doesnt work.
            if (function.aux != 0) {
                matteValue_t srcObject = {};
                srcObject.binIDreserved = MATTE_VALUE_TYPE_OBJECT;
                srcObject.value.id = function.aux;

                matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, function.value);
                if (stub && matte_bytecode_stub_is_dynamic_bind(stub)) {                    
                    matteValue_t dynName = matte_store_get_dynamic_bind_token(vm->store);
                    matte_array_push(args, srcObject);
                    matte_array_push(argnames, dynName);
                }
                
                
                if (stub && matte_value_object_get_is_interface_unsafe(vm->store, srcObject)) {
                    privateBinding = matte_value_object_get_interface_private_binding_unsafe(vm->store, srcObject);
                }
            }

            #ifdef MATTE_DEBUG__STORE
                matteString_t * info = matte_string_create_from_c_str("FUNCTION CALLED @");
                if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)))
                    matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
                matte_store_track_neutral(vm->store, function.value, matte_string_get_c_str(info), VM_EXECUTABLE_LOOP_CURRENT_LINE);
                matte_string_destroy(info);
            #endif
            
            matteValue_t result = matte_vm_call_full(vm, function.value, privateBinding, args, argnames, NULL);

            for(i = 0; i < argcount; ++i) {
                STACK_POP_NORET();
                STACK_POP_NORET(); // always a string
            }
            matte_array_destroy(args);
            matte_array_destroy(argnames);
            STACK_POP_NORET();
            STACK_PUSH(result);
            break;
          }
          case MATTE_OPCODE_ARF: {            
            if (STACK_SIZE() < 1) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for referrable assignment, but insufficient arguments on the stack.");    
                break;            
            }
            uint64_t refn = ((uint64_t)inst->data) % 0xffffffff;
            uint64_t op  = ((uint64_t)inst->data) / 0xffffffff;
            
            matteValue_t * ref = matte_vm_stackframe_get_referrable(vm, 0, refn); 
            if (ref) {
                matteValue_t v = STACK_PEEK(0);
                matteValue_t vOut;
                switch(op + (int)MATTE_OPERATOR_ASSIGNMENT_NONE) {
                  case MATTE_OPERATOR_ASSIGNMENT_NONE: {
                    matte_vm_stackframe_set_referrable(vm, 0, refn, v);
                    vOut = matte_store_new_value(vm->store);
                    matte_value_into_copy(vm->store, &vOut, v);
                    break;
                  }
                    
                  case MATTE_OPERATOR_ASSIGNMENT_ADD: vOut = vm_operator__assign_add(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_SUB: vOut = vm_operator__assign_sub(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_MULT: vOut = vm_operator__assign_mult(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_DIV: vOut = vm_operator__assign_div(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_MOD: vOut = vm_operator__assign_mod(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_POW: vOut = vm_operator__assign_pow(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_AND: vOut = vm_operator__assign_and(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_OR: vOut = vm_operator__assign_or(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_XOR: vOut = vm_operator__assign_xor(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_BLEFT: vOut = vm_operator__assign_bleft(vm, ref, v); break;
                  case MATTE_OPERATOR_ASSIGNMENT_BRIGHT: vOut = vm_operator__assign_bright(vm, ref, v); break;
                  default:
                    vOut = matte_store_new_value(vm->store);
                    matte_vm_raise_error_cstring(vm, "VM error: tried to access non-existent referrable operation (corrupt bytecode?).");                        

                }                
                STACK_POP_NORET();
                STACK_PUSH(vOut); // new value is pushed
            } else {
                matte_vm_raise_error_cstring(vm, "VM error: tried to access non-existent referrable.");    
            }
            break;
          }          
          case MATTE_OPCODE_POP: {
            uint32_t popCount = (uint32_t)inst->data;
            while (popCount && STACK_SIZE()) {
                matteValue_t m = STACK_POP();
                matte_store_recycle(vm->store, m);
                popCount--;
            }
            break;
          }    
          case MATTE_OPCODE_CPY: {
            if (!STACK_SIZE()) {
                matte_vm_raise_error_cstring(vm, "VM error: cannot CPY with empty stack");    
                break;
            }       
            matteValue_t m = STACK_PEEK(0);
            matteValue_t cpy = matte_store_new_value(vm->store);
            matte_value_into_copy(vm->store, &cpy, m);
            STACK_PUSH(cpy);
            break;
          }   
          case MATTE_OPCODE_OSN: {
            if (STACK_SIZE() < 3) {
                matte_vm_raise_error_cstring(vm, "VM error: OSN opcode requires 3 on the stack.");                
                break;        
            }
            int opr = (uint32_t)(inst->data);
            int isBracket = 0;
            if (opr >= (int)MATTE_OPERATOR_STATE_BRACKET) {
                opr -= MATTE_OPERATOR_STATE_BRACKET;
                isBracket = 1;
            }
            opr += (int)MATTE_OPERATOR_ASSIGNMENT_NONE;
            matteValue_t key    = STACK_PEEK(0);
            matteValue_t object = STACK_PEEK(1);
            matteValue_t val    = STACK_PEEK(2);

            if (opr == MATTE_OPERATOR_ASSIGNMENT_NONE) {
                
                matteValue_t lk = matte_value_object_set(vm->store, object, key, val, isBracket);
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_PUSH(lk);

            
            } else {
                matteValue_t * ref = matte_value_object_access_direct(vm->store, object, key, isBracket);
                matteValue_t refH = {};
                refH = matte_value_object_access(vm->store, object, key, isBracket);
                ref = &refH;
                matteValue_t out = matte_store_new_value(vm->store);
                switch(opr) {                    
                  case MATTE_OPERATOR_ASSIGNMENT_ADD: out = vm_operator__assign_add(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_SUB: out = vm_operator__assign_sub(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_MULT: out = vm_operator__assign_mult(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_DIV: out = vm_operator__assign_div(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_MOD: out = vm_operator__assign_mod(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_POW: out = vm_operator__assign_pow(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_AND: out = vm_operator__assign_and(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_OR: out = vm_operator__assign_or(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_XOR: out = vm_operator__assign_xor(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_BLEFT: out = vm_operator__assign_bleft(vm, ref, val); break;
                  case MATTE_OPERATOR_ASSIGNMENT_BRIGHT: out = vm_operator__assign_bright(vm, ref, val); break;
                  default:
                    matte_vm_raise_error_cstring(vm, "VM error: tried to access non-existent assignment operation (corrupt bytecode?).");                        
                }               
                
                // Slower path for things like accessors
                // Indirect access means the ref being worked with is essentially a copy, so 
                // we need to set the object value back after the operator has been applied.
                matte_value_object_set(
                    vm->store,
                    object,
                    key, 
                    refH,
                    isBracket
                );
                if (matte_value_type(refH)) { 
                    matte_store_recycle(vm->store, refH); 
                }
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_PUSH(out);
            }
            break;
          }    

          case MATTE_OPCODE_OLK: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: OLK opcode requires 2 on the stack.");                
                break;        
            }
            
            uint32_t isBracket = (uint32_t)inst->data;
            matteValue_t key = STACK_PEEK(0);
            matteValue_t object = STACK_PEEK(1);            
            matteValue_t output = matte_value_object_access(vm->store, object, key, isBracket);

            
            
            STACK_POP_NORET();
            STACK_POP_NORET();

            matteValue_Extended_t ve = {};
            ve.value = output;

            // if a dynamic binding OR private interface accessor, cache the binding
            if (matte_value_is_function(output) && matte_value_type(key) == MATTE_VALUE_TYPE_STRING && matte_value_type(object) == MATTE_VALUE_TYPE_OBJECT) {
                ve.aux = object.value.id;
            }        
            STACK_PUSH_EXTENDED(ve);


            break;
          }    


          case MATTE_OPCODE_EXT: {
            uint64_t call = (uint64_t)inst->data;
            if (call >= matte_array_get_size(vm->externalFunctionIndex)) {
                matte_vm_raise_error_cstring(vm, "VM error: unknown external call.");                
                break;        
            }
            matteValue_t fn = matte_array_at(vm->extFuncs, matteValue_t, call);
            STACK_PUSH(fn);
            break;
          }
          case MATTE_OPCODE_RET: {
            // ez pz
            frame->pc = instCount;
            break;
          }
          // used to implement all branching
          case MATTE_OPCODE_SKP: {
            uint32_t count = (uint32_t)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (!matte_value_as_boolean(vm->store, condition)) {
                frame->pc += count;
            }
            STACK_POP_NORET();
            break;
          }
          case MATTE_OPCODE_SCA: {
            uint32_t count = (uint32_t)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (!matte_value_as_boolean(vm->store, condition)) {
                frame->pc += count;
            }
            break;
          }
          case MATTE_OPCODE_SCO: {
            uint32_t count = (uint32_t)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (matte_value_as_boolean(vm->store, condition)) {
                frame->pc += count;
            }
            break;
          }

          case MATTE_OPCODE_FVR: {
            matteValue_t a    = STACK_PEEK(0);
            if (!matte_value_is_callable(vm->store, a)) {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "'forever' requires only argument to be a function."));
                STACK_POP_NORET();          
                return matte_store_new_value(vm->store);
            }

            vm->pendingRestartCondition = vm_ext_call__forever_restart_condition;
            matte_store_recycle(vm->store, matte_vm_call(vm, a, matte_array_empty(),matte_array_empty(), NULL));
            STACK_POP_NORET();          
            break;
          }    
          
          case MATTE_OPCODE_FCH: {
            matteValue_t b = STACK_PEEK(0);
            matteValue_t a = STACK_PEEK(1);
            
            if (!matte_value_is_callable(vm->store, b)) {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "'foreach' requires the expression after it to reduce to a function."));
                STACK_POP_NORET();          
                STACK_POP_NORET();                    
                return matte_store_new_value(vm->store);
            }

            if (matte_value_type(a) != MATTE_VALUE_TYPE_OBJECT) {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "'foreach' requires an object."));
                STACK_POP_NORET();          
                STACK_POP_NORET();                    
                return matte_store_new_value(vm->store);
            }


            matte_value_object_push_lock(vm->store, a);
            matte_value_object_push_lock(vm->store, b);
            matte_value_object_foreach(vm->store, a, b);
            matte_value_object_pop_lock(vm->store, a);
            matte_value_object_pop_lock(vm->store, b);
            STACK_POP_NORET();                    
            STACK_POP_NORET();                    
            break;
          }
                
          case MATTE_OPCODE_LOP: {
            matteValue_t from = STACK_PEEK(2);
            matteValue_t to   = STACK_PEEK(1);
            matteValue_t v    = STACK_PEEK(0);
          
          
            
            if (!matte_value_is_function(v)) {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "'for' requires trailing expression to reduce to a function"));    
                STACK_POP_NORET();          
                STACK_POP_NORET();          
                STACK_POP_NORET();          
                return matte_store_new_value(vm->store);
            }

            ForLoopData d = {
                matte_value_as_number(vm->store, from),
                matte_value_as_number(vm->store, to),
                1,
                matte_bytecode_stub_arg_count(matte_value_get_bytecode_stub(vm->store, v)) != 0
            };
            if (d.i == d.end) {
                STACK_POP_NORET();          
                STACK_POP_NORET();          
                STACK_POP_NORET(); 
                break;                     
            }
            

            if (d.i >= d.end) {
                vm->pendingRestartCondition = vm_ext_call__for_restart_condition__down;
                d.offset = -1;
            } else {
                vm->pendingRestartCondition = vm_ext_call__for_restart_condition__up;
            }

            vm->pendingRestartConditionData = &d;
            matteValue_t iter = matte_store_new_value(vm->store);
            matte_value_into_number(vm->store, &iter, d.i);

            // dynamically bind the first name
            // We can't reasonable expect to know what the user places 
            // as their argument, as it is really common to have 
            // embedded loops, so it cannot be static. 
            matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, v);
            matteValue_t firstArgName = vm->specialString_value;
            matteArray_t arr;
            matteArray_t arrNames;
            
            if (matte_bytecode_stub_arg_count(stub)) {
                firstArgName = matte_bytecode_stub_get_arg_name(stub, 0);
                arr = MATTE_ARRAY_CAST(&iter, matteValue_t, 1);
                arrNames = MATTE_ARRAY_CAST(&firstArgName, matteValue_t, 1);
            } else {
                arr = *matte_array_empty();
                arrNames = *matte_array_empty();
            }



            matteValue_t result = matte_vm_call(vm, v, &arr, &arrNames, NULL);
            STACK_POP_NORET();          
            STACK_POP_NORET();          
            STACK_POP_NORET(); 
            break;         
          }
          
          case MATTE_OPCODE_OAS: {
            matteValue_t src  = STACK_PEEK(0);
            matteValue_t dest = STACK_PEEK(1);
            
            matte_value_object_set_table(vm->store, dest, src);
            
            
            STACK_POP_NORET();
            break;
          }

          case MATTE_OPCODE_ASP: {
            uint32_t count = (uint32_t)inst->data;
            frame->pc += count;
            break;
          }  
          
          case MATTE_OPCODE_QRY: {
            matteValue_t o = STACK_PEEK(0);
            matteValue_t output = matte_value_query(vm->store, &o, (matteQuery_t)inst->data);
            o = STACK_POP();            

            STACK_PUSH(output);
            // re-insert the base as "base"
            if (matte_value_is_function(output)) {
                STACK_PUSH(o);
                matteValue_t vv = matte_store_new_value(vm->store);
                matte_value_into_copy(vm->store, &vv, vm->specialString_base);
                STACK_PUSH(vv);
            } else {
                matte_store_recycle(vm->store, o);
            }
            
            break;
          }
          
          
          case MATTE_OPCODE_OPR: {            
            switch((int)inst->data) {
                case MATTE_OPERATOR_ADD:
                case MATTE_OPERATOR_SUB:
                case MATTE_OPERATOR_DIV:
                case MATTE_OPERATOR_MULT:
                case MATTE_OPERATOR_BITWISE_OR:
                case MATTE_OPERATOR_OR:
                case MATTE_OPERATOR_BITWISE_AND:
                case MATTE_OPERATOR_AND:
                case MATTE_OPERATOR_SHIFT_LEFT:
                case MATTE_OPERATOR_SHIFT_RIGHT:
                case MATTE_OPERATOR_POW:
                case MATTE_OPERATOR_EQ:
                case MATTE_OPERATOR_POINT:
                case MATTE_OPERATOR_TERNARY:
                case MATTE_OPERATOR_GREATER:
                case MATTE_OPERATOR_LESS:
                case MATTE_OPERATOR_GREATEREQ:
                case MATTE_OPERATOR_LESSEQ:
                case MATTE_OPERATOR_TRANSFORM:
                case MATTE_OPERATOR_MODULO:
                case MATTE_OPERATOR_CARET:
                case MATTE_OPERATOR_TYPESPEC:
                case MATTE_OPERATOR_NOTEQ: {
                    if (STACK_SIZE() < 2) {
                        matte_vm_raise_error_cstring(vm, "OPR operator requires 2 operands.");                        
                    } else {
                        matteValue_t a = STACK_PEEK(1);
                        matteValue_t b = STACK_PEEK(0);
                        matteValue_t v = vm_operator_2(
                            vm,
                            (matteOperator_t)(inst->data),
                            a, b
                        );
                        STACK_POP_NORET();
                        STACK_POP_NORET();
                        STACK_PUSH(v); // ok
                    }
                    break;                
                }
                    
                
                case MATTE_OPERATOR_NOT:
                case MATTE_OPERATOR_NEGATE:
                case MATTE_OPERATOR_BITWISE_NOT:
                case MATTE_OPERATOR_POUND:{
                    if (STACK_SIZE() < 1) {
                        matte_vm_raise_error_cstring(vm, "OPR operator requires 1 operand.");                        
                    } else {
                    
                        matteValue_t a = STACK_PEEK(0);
                        matteValue_t v = vm_operator_1(
                            vm,
                            (matteOperator_t)inst->data,
                            a
                        );
                        STACK_POP_NORET();
                        STACK_PUSH(v);
                    }
                    break;                
                }
            }
            break;
          }
      
          default: 
            matte_vm_raise_error_cstring(vm, "Unknown / unhandled opcode."); 
        
        }


        // catchables are checked after every instruction.
        // once encountered they are handled immediately.
        // If there is no handler, the catchable propogates down the callstack.
        if (vm->pendingCatchable) {
            break;
        } 
        
    }


    
    matteValue_t output;
    // top of stack is output
    if (matte_array_get_size(frame->valueStack)) {
        
        output = matte_array_at(frame->valueStack, matteValue_Extended_t, matte_array_get_size(frame->valueStack)-1).value;
        uint32_t i;
        uint32_t len = matte_array_get_size(frame->valueStack);
        for(i = 0; i < len-1; ++i) { 
            matte_store_recycle(vm->store, matte_array_at(frame->valueStack, matteValue_Extended_t, i).value);
        }
        matte_array_set_size(frame->valueStack, 0);

        if (vm->pendingCatchable) 
            output = matte_store_new_value(vm->store);
        
        // ok since not removed from normal value stack stuff
    } else {
        output = matte_store_new_value(vm->store);
    }

    // it's VERY important that the restart condition is not run when 
    // pendingCatchable is true, as the stack value top does not correspond
    // to anything meaningful
    if (frame->restartCondition && !vm->pendingCatchable) {
        if (frame->restartCondition(vm, frame, output, frame->restartConditionData)) {
            frame->pc = 0;
            goto RELOOP;
        }
    }
    vm_execution_loop__stack_depth--;
    return output;
}

#define WRITE_BYTES(__T__, __VAL__) matte_array_push_n(arr, &(__VAL__), sizeof(__T__));
#define WRITE_NBYTES(__VAL__, __N__) matte_array_push_n(arr, &(__VAL__), __N__);

static void write_unistring(matteArray_t * arr, matteString_t * str) {
    uint32_t len = matte_string_get_utf8_length(str);
    WRITE_BYTES(uint32_t, len);
    WRITE_NBYTES(((uint8_t*)matte_string_get_utf8_data(str))[0], len);
}


static void vm_add_built_in(
    matteVM_t * vm,
    uint32_t index, 
    const matteArray_t * argNames, 
    matteValue_t (*cb)(matteVM_t *, matteValue_t v, const matteValue_t * args, void * userData)
) {
    ExternalFunctionSet_t * set;
    if (matte_array_get_size(vm->externalFunctionIndex) <= index) {
        matte_array_set_size(vm->externalFunctionIndex, index+1);
    }
    if (matte_array_get_size(vm->extStubs) <= index) {
        matte_array_set_size(vm->extStubs, index+1);
        matte_array_set_size(vm->extFuncs, index+1);
    }

    set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, index);
    set->userFunction = cb;
    set->userData = NULL;
    set->nArgs = matte_array_get_size(argNames);

    
    matteArray_t * arr = matte_array_create(1);
    uint8_t tag[] = {
        'M', 'A', 'T', 0x01, 0x06, 'B', 0x1
    };
    uint8_t u8;
    uint32_t u32 = index;
    WRITE_NBYTES(tag, 7); 
    WRITE_BYTES(uint32_t, u32); 
    u8 = 0;
    WRITE_BYTES(uint8_t, u8); // varargs
    
    u8 = matte_array_get_size(argNames);
    WRITE_BYTES(uint8_t, u8); 
    uint32_t i;
    for(i = 0; i < u8; ++i) {
        write_unistring(arr, matte_array_at(argNames, matteString_t *, i));
    }
    
    
    matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
        vm->store,
        0,
        (const uint8_t*) matte_array_get_data(arr), 
        matte_array_get_size(arr)
    );
    matte_array_destroy(arr);    
    matteBytecodeStub_t * out = matte_array_at(stubs, matteBytecodeStub_t *, 0);

    matte_array_destroy(stubs);
    matte_array_at(vm->extStubs, matteBytecodeStub_t *, index) = out;

    matteValue_t func = {};
    matte_value_into_new_external_function_ref(vm->store, &func, out);
    matte_array_at(vm->extFuncs, matteValue_t, index) = func;
}

#include "MATTE_EXT_NUMBER"
#include "MATTE_EXT_OBJECT"
#include "MATTE_EXT_STRING"
matteVM_t * matte_vm_create(matte_t * m) {
    matteVM_t * vm = (matteVM_t*)matte_allocate(sizeof(matteVM_t));
    vm->matte = m;

    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t *));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    vm->externalFunctions = matte_table_create_hash_matte_string();
    vm->externalFunctionIndex = matte_array_create(sizeof(ExternalFunctionSet_t));
    vm->store = matte_store_create(vm);

    vm->extStubs = matte_array_create(sizeof(matteBytecodeStub_t *));
    vm->extFuncs = matte_array_create(sizeof(matteValue_t));
    vm->importName2ID = matte_table_create_hash_matte_string();
    vm->id2importName = matte_table_create_hash_pointer();
    vm->imported = matte_table_create_hash_pointer();
    vm->nextID = 1;
    vm->cleanupFunctionSets = matte_array_create(sizeof(MatteCleanupFunctionSet));
    
    vm->specialString_from = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_from, MATTE_VM_STR_CAST(vm, "from"));
    vm->specialString_message = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_message, MATTE_VM_STR_CAST(vm, "message"));
    vm->specialString_parameters = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_parameters, MATTE_VM_STR_CAST(vm, "parameters"));
    vm->specialString_base = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_base, MATTE_VM_STR_CAST(vm, "base"));


    vm->specialString_onsend = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_onsend, MATTE_VM_STR_CAST(vm, "onSend"));
    vm->specialString_onerror = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_onerror, MATTE_VM_STR_CAST(vm, "onError"));

    
    // add built in functions
    const matteString_t * forever_name = MATTE_VM_STR_CAST(vm, "do");
    const matteString_t * query_name = MATTE_VM_STR_CAST(vm, "base");
    const matteString_t * for_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "do")
    };
    const matteString_t * import_names[] = {
        MATTE_VM_STR_CAST(vm, "module"),
        MATTE_VM_STR_CAST(vm, "parameters"),
        MATTE_VM_STR_CAST(vm, "alias"),
        MATTE_VM_STR_CAST(vm, "preloadOnly")
    };
    const matteString_t * keyName = MATTE_VM_STR_CAST(vm, "key");
    const matteString_t * keys = MATTE_VM_STR_CAST(vm, "keys");
    const matteString_t * removeKey_names[] = {
        query_name,
        keyName,
        keys
    };
    const matteString_t * type_names[] = {
        MATTE_VM_STR_CAST(vm, "name"),
        MATTE_VM_STR_CAST(vm, "inherits"),
        MATTE_VM_STR_CAST(vm, "layout"),
    };
    const matteString_t * of = MATTE_VM_STR_CAST(vm, "of");
    const matteString_t * setAttributes_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "attributes")
    };
    const matteString_t * type = MATTE_VM_STR_CAST(vm, "type");
    const matteString_t * message = MATTE_VM_STR_CAST(vm, "message");
    const matteString_t * detail = MATTE_VM_STR_CAST(vm, "detail");

    const matteString_t * name = MATTE_VM_STR_CAST(vm, "name");
    const matteString_t * value = MATTE_VM_STR_CAST(vm, "value");
    vm->specialString_value = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_value, value);
    const matteString_t * previous = MATTE_VM_STR_CAST(vm, "previous");
    vm->specialString_previous = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &vm->specialString_previous, previous);

    const matteString_t * stringName = MATTE_VM_STR_CAST(vm, "string");
    
    const matteString_t * charAt_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "index")
    };
    const matteString_t * charAtSet_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "index"),
        value
    };


    const matteString_t * subset_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "from"),
        MATTE_VM_STR_CAST(vm, "to")
    };
    const matteArray_t * emptyArr = matte_array_empty();

    const matteString_t * push_names[] = {
        query_name,
        value
    };

    const matteString_t * setsize_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "size")
    };

    const matteString_t * insert_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "at"),
        value,
    };
    
    const matteString_t * comparator = MATTE_VM_STR_CAST(vm, "comparator");
    const matteString_t * sort_names[] = {
        query_name,
        comparator
    };
    
    const matteString_t * functional_names[] = {
        query_name,
        forever_name
    };

    const matteString_t * conditional_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "condition"),
    };


    const matteString_t * interface_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "enabled"),
        MATTE_VM_STR_CAST(vm, "private"),
    };
    
    const matteString_t * findIndex_names[] = {
        query_name,        
        value,
        MATTE_VM_STR_CAST(vm, "query")        
    };
    const matteString_t * is_names[] = {
        query_name,
        type,
    };
    const matteString_t * search_names[] = {
        query_name,
        keyName
    };
    const matteString_t * replace_names[] = {
        query_name,
        keyName,
        MATTE_VM_STR_CAST(vm, "with"),
        keys
    };
    
    const matteString_t * strings = MATTE_VM_STR_CAST(vm, "strings");
    const matteString_t * token = MATTE_VM_STR_CAST(vm, "token");

    const matteString_t * substr_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "from"),
        MATTE_VM_STR_CAST(vm, "to")
    };


    
    const matteString_t * splitNames[] = {
        query_name,
        token
    };

    const matteString_t * scanNames[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "format")
    };

    const matteString_t * formatNames[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "items")
    };
    
    const matteString_t * atan2Names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "y")
    };
    
    const matteString_t * filterNames[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "by")
    };

    const matteString_t * mapReduceNames[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "to")
    };



    matteArray_t temp;
    vm_add_built_in(vm, MATTE_EXT_CALL_NOOP,  matte_array_empty(), vm_ext_call__noop);
    vm_add_built_in(vm, MATTE_EXT_CALL_BREAKPOINT,  matte_array_empty(), vm_ext_call__breakpoint);
    temp = MATTE_ARRAY_CAST(&import_names, matteString_t *, 4);vm_add_built_in(vm, MATTE_EXT_CALL_IMPORT,  &temp, vm_ext_call__import);

    temp = MATTE_ARRAY_CAST(&message, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_PRINT,      &temp, vm_ext_call__print);
    temp = MATTE_ARRAY_CAST(&message, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_SEND,       &temp, vm_ext_call__send);
    temp = MATTE_ARRAY_CAST(&detail, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_ERROR,      &temp, vm_ext_call__error);

    temp = MATTE_ARRAY_CAST(&name, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_GETEXTERNALFUNCTION, &temp, vm_ext_call__getexternalfunction);



    // NUMBERS
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__PI,        &temp, vm_ext_call__number__pi);    
    temp = MATTE_ARRAY_CAST(&stringName, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__PARSE,     &temp, vm_ext_call__number__parse);    
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__RANDOM,    &temp, vm_ext_call__number__random);    


    // STRING
    temp = MATTE_ARRAY_CAST(&strings, matteString_t *, 1);   vm_add_built_in(vm, MATTE_EXT_CALL__STRING__COMBINE,     &temp, vm_ext_call__string__combine);    



    // OBJECTS
    temp = MATTE_ARRAY_CAST(type_names, matteString_t *, 3);  vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__NEWTYPE,     &temp, vm_ext_call__object__newtype);    
    temp = MATTE_ARRAY_CAST(&type, matteString_t *, 1);   vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__INSTANTIATE,     &temp, vm_ext_call__object__instantiate);    
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__FREEZEGC,    &temp, vm_ext_call__object__freeze_gc);    
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__THAWGC,    &temp, vm_ext_call__object__thaw_gc);    
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__GARBAGECOLLECT,    &temp, vm_ext_call__object__garbage_collect);    

    
    
    
    
    

    // QUERY: NUMBER
    temp = MATTE_ARRAY_CAST(atan2Names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ATAN2,       &temp, vm_ext_call__number__atan2);    

    // QUERY: OBJECT 
    temp = MATTE_ARRAY_CAST(push_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__PUSH,     &temp, vm_ext_call__object__push);    
    temp = MATTE_ARRAY_CAST(setsize_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SETSIZE,     &temp, vm_ext_call__object__setsize);    
    temp = MATTE_ARRAY_CAST(insert_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__INSERT,     &temp, vm_ext_call__object__insert);    
    temp = MATTE_ARRAY_CAST(removeKey_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REMOVE,     &temp, vm_ext_call__object__remove);    
    temp = MATTE_ARRAY_CAST(setAttributes_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SETATTRIBUTES,     &temp, vm_ext_call__object__set_attributes);    
    temp = MATTE_ARRAY_CAST(sort_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SORT,     &temp, vm_ext_call__object__sort);    
    temp = MATTE_ARRAY_CAST(subset_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SUBSET,     &temp, vm_ext_call__object__subset);    
    temp = MATTE_ARRAY_CAST(filterNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FILTER,     &temp, vm_ext_call__object__filter);    
    temp = MATTE_ARRAY_CAST(findIndex_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FINDINDEX,     &temp, vm_ext_call__object__findindex);    
    temp = MATTE_ARRAY_CAST(is_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ISA,     &temp, vm_ext_call__object__is);    
    temp = MATTE_ARRAY_CAST(mapReduceNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__MAP,     &temp, vm_ext_call__object__map);    
    temp = MATTE_ARRAY_CAST(mapReduceNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REDUCE,     &temp, vm_ext_call__object__reduce);    
    temp = MATTE_ARRAY_CAST(conditional_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ANY,     &temp, vm_ext_call__object__any);    
    temp = MATTE_ARRAY_CAST(conditional_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ALL,     &temp, vm_ext_call__object__all);    
    temp = MATTE_ARRAY_CAST(for_names, matteString_t *, 2);vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FOREACH, &temp, vm_ext_call__object__foreach);
    temp = MATTE_ARRAY_CAST(interface_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SET_IS_INTERFACE,     &temp, vm_ext_call__object__set_is_interface);    

    
    
    // QUERY: STRING 
    
    temp = MATTE_ARRAY_CAST(search_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SEARCH,     &temp, vm_ext_call__string__search);    
    temp = MATTE_ARRAY_CAST(search_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SEARCH_ALL,     &temp, vm_ext_call__string__search_all);    
    temp = MATTE_ARRAY_CAST(search_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__CONTAINS,     &temp, vm_ext_call__string__contains);    
    temp = MATTE_ARRAY_CAST(replace_names, matteString_t *, 4);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REPLACE,     &temp, vm_ext_call__string__replace);    
    temp = MATTE_ARRAY_CAST(search_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__COUNT,     &temp, vm_ext_call__string__count);    
    temp = MATTE_ARRAY_CAST(charAt_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__CHARCODEAT,     &temp, vm_ext_call__string__charcodeat);    
    temp = MATTE_ARRAY_CAST(charAt_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__CHARAT,     &temp, vm_ext_call__string__charat);    
    temp = MATTE_ARRAY_CAST(charAtSet_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SETCHARCODEAT,     &temp, vm_ext_call__string__setcharcodeat);    
    temp = MATTE_ARRAY_CAST(charAtSet_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SETCHARAT,     &temp, vm_ext_call__string__setcharat);    
    temp = MATTE_ARRAY_CAST(charAt_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REMOVECHAR,     &temp, vm_ext_call__string__removechar);    
    temp = MATTE_ARRAY_CAST(substr_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SUBSTR,     &temp, vm_ext_call__string__substr);    
    temp = MATTE_ARRAY_CAST(splitNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SPLIT,     &temp, vm_ext_call__string__split);    
    temp = MATTE_ARRAY_CAST(scanNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SCAN,     &temp, vm_ext_call__string__scan);    
    temp = MATTE_ARRAY_CAST(formatNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FORMAT,      &temp, vm_ext_call__string__format);    

    
    
    



    //vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_NOWRITE, 0, vm_ext_call__introspect_nowrite);    
    matte_bind_native_functions(vm);



    
    // add the default roms
    {
        uint32_t i;
        uint32_t len = MATTE_ROM__COUNT;
        for(i = 0; i < len; ++i) {
            matteString_t * name = matte_string_create_from_c_str("%s", MATTE_ROM__names[i]);

            // found!
            uint32_t srcLen = MATTE_ROM__sizes[i];
            // promise to be safe
            uint8_t * src = (uint8_t*)MATTE_ROM__data + MATTE_ROM__offsets[i];

            uint32_t fileid = matte_vm_get_new_file_id(vm, name);
            
            matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
                vm->store,
                fileid,
                src,
                srcLen
            );        
            matte_string_destroy(name);    
            matte_vm_add_stubs(vm, stubs);
            matte_array_destroy(stubs);
        }
   }

    // pre-allocate lookahead
    vm_push_frame(vm);
    vm_pop_frame(vm);
    return vm;
}

void matte_vm_destroy(matteVM_t * vm) {
    uint32_t i;
    uint32_t len = matte_array_get_size(vm->cleanupFunctionSets);
    for(i = 0; i < len; ++i) {
        MatteCleanupFunctionSet f = matte_array_at(vm->cleanupFunctionSets, MatteCleanupFunctionSet, i);
        f.fn(vm, f.data);
    }
    matte_array_destroy(vm->cleanupFunctionSets);
    if (vm->pendingCatchable) {
        matte_value_object_pop_lock(vm->store, vm->catchable);
        matte_store_recycle(vm->store, vm->catchable);
        vm->catchable.binIDreserved = 0;
    }
    

    matteTableIter_t * iter = matte_table_iter_create();
    matteTableIter_t * subiter = matte_table_iter_create();

    for(matte_table_iter_start(iter, vm->imported);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matteValue_t * v = (matteValue_t*)matte_table_iter_get_value(iter);
        matte_value_object_pop_lock(vm->store, *v);
        matte_store_recycle(vm->store, *v);
        matte_deallocate(v);
    }

    len = matte_array_get_size(vm->extFuncs);
    matte_array_destroy(vm->extFuncs);



    matte_table_destroy(vm->imported);
    matte_store_destroy(vm->store);
    

    

    for(matte_table_iter_start(iter, vm->stubIndex);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matteTable_t * table = (matteTable_t*)matte_table_iter_get_value(iter);

        for(matte_table_iter_start(subiter, table);
            !matte_table_iter_is_end(subiter);
            matte_table_iter_proceed(subiter)) {
            matte_bytecode_stub_destroy((matteBytecodeStub_t*)matte_table_iter_get_value(subiter));
        }

        matte_table_destroy(table);
    }
    matte_table_destroy(vm->stubIndex);




    for(matte_table_iter_start(iter, vm->id2importName);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matte_string_destroy((matteString_t*)matte_table_iter_get_value(iter));
    }
    matte_table_destroy(vm->id2importName);

    for(matte_table_iter_start(iter, vm->importName2ID);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matte_deallocate(matte_table_iter_get_value(iter));
    }
    matte_table_destroy(vm->importName2ID);


    for(matte_table_iter_start(iter, vm->externalFunctions);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matte_deallocate(matte_table_iter_get_value(iter));
    }
    
    matte_table_destroy(vm->externalFunctions);




    len = matte_array_get_size(vm->callstack);
    for(i = 0; i < len; ++i) {
        matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t *, i);
        matte_string_destroy(frame->prettyName);
        matte_array_destroy(frame->valueStack);
        matte_array_destroy(frame->referrables);
        matte_deallocate(frame);
    }

    len = matte_array_get_size(vm->extStubs);
    for(i = 0; i < len; ++i) {
        matte_bytecode_stub_destroy(matte_array_at(vm->extStubs, matteBytecodeStub_t *, i));
    }
    matte_array_destroy(vm->extStubs);


    matte_array_destroy(vm->interruptOps);
    matte_array_destroy(vm->callstack);
    matte_array_destroy(vm->externalFunctionIndex); // copy, safe
    matte_table_iter_destroy(iter);
    matte_table_iter_destroy(subiter);

    for(i = 0; i < matte_string_temp_max_calls; ++i) {
        if (vm->string_tempVals[i])
            matte_string_destroy(vm->string_tempVals[i]);
    }
    matte_deallocate(vm);
}

const matteString_t * matte_vm_get_script_name_by_id(matteVM_t * vm, uint32_t fileid) {
    return (matteString_t*)matte_table_find_by_uint(vm->id2importName, fileid);
}

void matte_vm_add_stubs(matteVM_t * vm, const matteArray_t * arr) {
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    for(i = 0; i < len; ++i) {
        matteBytecodeStub_t * stub = matte_array_at(arr, matteBytecodeStub_t *, i);
        matteTable_t * fileindex = (matteTable_t*)matte_table_find_by_uint(vm->stubIndex, matte_bytecode_stub_get_file_id(stub));
        if (!fileindex) {
            fileindex = matte_table_create_hash_pointer();
            matte_table_insert_by_uint(vm->stubIndex, matte_bytecode_stub_get_file_id(stub), fileindex);
        }
        
        matte_table_insert_by_uint(fileindex, matte_bytecode_stub_get_id(stub), stub);
    }
}

matteStore_t * matte_vm_get_store(matteVM_t * vm) {return vm->store;}


matteValue_t matte_vm_call_full(
    matteVM_t * vm, 
    matteValue_t func, 
    matteValue_t privateBinding,
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
) {
    if (vm->pendingCatchable) return matte_store_new_value(vm->store);
    if (matte_value_is_empty_function(func)) {
        vm->pendingRestartCondition = NULL;
        vm->pendingRestartConditionData = NULL;
        return matte_store_new_value(vm->store);
    }
    int callable = matte_value_is_callable(vm->store, func); 
    if (!callable) {
        matteString_t * err = matte_string_create_from_c_str("Error: cannot call non-function value ");
        if (prettyName)
            matte_string_concat(err, prettyName);
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_store_new_value(vm->store);
    }

    if (matte_array_get_size(args) != matte_array_get_size(argNames)) {
        matte_vm_raise_error_cstring(vm, "VM call as mismatching arguments and parameter names.");            
        return matte_store_new_value(vm->store);
    }

    // special case: type object as a function
    if (matte_value_type(func) == MATTE_VALUE_TYPE_TYPE) {                
        if (matte_array_get_size(args)) {
            if (matte_array_at(argNames, matteValue_t, 0).value.id != vm->specialString_from.value.id) {
                matte_vm_raise_error_cstring(vm, "Type conversion failed: unbound parameter to function ('from')");            
            }
            return matte_value_to_type(vm->store, matte_array_at(args, matteValue_t, 0), func);
        } else {
            matte_vm_raise_error_cstring(vm, "Type conversion failed (no value given to convert).");
            return matte_store_new_value(vm->store);
        }
    }


    // normal function
    matteValue_t d = matte_store_new_value(vm->store);
    if (matte_value_object_function_was_activated(vm->store, func)) {
        matte_value_into_cloned_function_ref(vm->store, &d, func);
    } else {
        matte_value_into_copy(vm->store, &d, func);    
    }


    if (matte_bytecode_stub_get_file_id(matte_value_get_bytecode_stub(vm->store, d)) == 0) {
        // fileid 0 is a special fileid that never refers to a real file.
        // this is used to call external c functions. stubID refers to 
        // which index within externalFunction.
        // In this case, a new stackframe is NOT pushed.
        uint32_t external = matte_bytecode_stub_get_id(matte_value_get_bytecode_stub(vm->store, d));
        if (external >= matte_array_get_size(vm->externalFunctionIndex)) {
            matte_store_recycle(vm->store, d);
            return matte_store_new_value(vm->store);            
        }
        ExternalFunctionSet_t * set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, external);
        matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, func);
        matteArray_t * argsReal = matte_array_create(sizeof(matteValue_t));
        uint32_t i, n;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(stub);
        matte_array_set_size(argsReal, len);
        // empty for any unset values.
        memset(matte_array_get_data(argsReal), 0, sizeof(matteValue_t)*len);
        
        // external functions are never var arg functions.
        for(i = 0; i < lenReal; ++i) {
            for(n = 0; n < len; ++n) {
                if (matte_bytecode_stub_get_arg_name(stub, n).value.id == matte_array_at(argNames, matteValue_t, i).value.id) {
                    matteValue_t v = matte_array_at(args, matteValue_t, i);
                    matte_array_at(argsReal, matteValue_t, n) = v;
                    // sicne this function doesn't use a referrable, we need to set roots manually.
                    if (matte_value_type(v) == MATTE_VALUE_TYPE_OBJECT) {
                        matte_value_object_push_lock(vm->store, v);
                    } else if (matte_value_type(v) == MATTE_VALUE_TYPE_STRING) {
                        matteValue_t vv = matte_store_new_value(vm->store);
                        matte_value_into_copy(vm->store, &vv, v);
                        matte_array_at(argsReal, matteValue_t, n) = vv;                    
                    }

                    break;
                }
            }       
            
            if (n == len) {
                matteString_t * str;
                // couldnt find the requested name. Throw an error.
                if (len)
                    str = matte_string_create_from_c_str(
                        "Could not bind requested parameter: '%s'.\n Bindable parameters for this function: ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->store, matte_array_at(argNames, matteValue_t, i)))
                    );
                else {
                    str = matte_string_create_from_c_str(
                        "Could not bind requested parameter: '%s'.\n (no bindable parameters for this function) ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->store, matte_array_at(argNames, matteValue_t, i)))
                    );                
                }

                for(n = 0; n < len; ++n) {
                    matteValue_t name = matte_bytecode_stub_get_arg_name(stub, n);
                    matte_string_concat_printf(str, " \"");
                    matte_string_concat(str, matte_value_string_get_string_unsafe(vm->store, name));
                    matte_string_concat_printf(str, "\" ");
                }
                
                matte_vm_raise_error_string(vm, str);
                matte_string_destroy(str);
                
                matte_array_destroy(argsReal);
                return matte_store_new_value(vm->store);
                
            }
        }
        
        
        
        matte_value_object_push_lock(vm->store, d);
        matteValue_t result = {};
        if (callable == 2) {
            int ok = matte_value_object_function_pre_typecheck_unsafe(vm->store, d, argsReal);
            if (ok) 
                result = set->userFunction(vm, d, (matteValue_t*)matte_array_get_data(argsReal), set->userData);
            matte_value_object_function_post_typecheck_unsafe(vm->store, d, result);
        } else {
            result = set->userFunction(vm, d, (matteValue_t*)matte_array_get_data(argsReal), set->userData);        
        }
        matte_value_object_pop_lock(vm->store, d);


        len = matte_array_get_size(argsReal);
        for(i = 0; i < len; ++i) {
            int bid = matte_value_type(matte_array_at(argsReal, matteValue_t, i));
            if (bid == MATTE_VALUE_TYPE_OBJECT) {
                matte_value_object_pop_lock(vm->store, matte_array_at(argsReal, matteValue_t, i));
            }
            matte_store_recycle(vm->store, matte_array_at(argsReal, matteValue_t, i));
        }
        matte_array_destroy(argsReal);
        matte_store_recycle(vm->store, d);
        return result;
    }
    matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->store, d);
    uint32_t instCount;
    matte_bytecode_stub_get_instructions(stub, &instCount); 

    // Fast path -> empty function
    if (instCount == 0) return matte_store_new_value(vm->store);
    
    
    matteVMStackFrame_t * prevFrame = vm->stacksize == 0 ? NULL :
        matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);                
    
    {
        // prepare future frame by looking ahead slightly 
        // and preparing its referrables.
        matteArray_t * referrables = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize)->referrables;
        referrables->size = 0;

        // slot 0 is always the context
        uint32_t i, n;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(stub);
        matte_array_set_size(referrables, 1+len);
        // empty for any unset values.
        memset(matte_array_get_data(referrables), 0, sizeof(matteValue_t)*(len+1));

        if (matte_bytecode_stub_is_vararg(stub)) {
            // var arg functions have a single argument
            // that are prepared from the calling args.
            matteValue_t val = matte_store_new_value(vm->store);
            matte_value_into_new_object_ref(vm->store, &val);
            
            for(i = 0; i < lenReal; ++i) {
                matte_value_object_set(
                    vm->store,
                    val,
                    matte_array_at(argNames, matteValue_t, i),
                    matte_array_at(args, matteValue_t, i),
                    0
                );
            }
            matte_array_at(referrables, matteValue_t, 1) = val;
            
        } else {
            for(i = 0; i < lenReal; ++i) {
                for(n = 0; n < len; ++n) {
                    if (matte_bytecode_stub_get_arg_name(stub, n).value.id == matte_array_at(argNames, matteValue_t, i).value.id) {
                        matte_array_at(referrables, matteValue_t, n+1) = matte_array_at(args, matteValue_t, i);
                        break;
                    }
                }       
                
                if (n == len) {
                    // couldnt find the requested name. Throw an error.
                    matteString_t * str;
                    if (len) 
                        str = matte_string_create_from_c_str(
                            "Could not bind requested parameter: '%s'.\n Bindable parameters for this function: ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->store, matte_array_at(argNames, matteValue_t, i)))
                        );
                    else {
                        str = matte_string_create_from_c_str(
                            "Could not bind requested parameter: '%s'.\n (no bindable parameters for this function) ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->store, matte_array_at(argNames, matteValue_t, i)))
                        );                
                    }
                    
                    
                    for(n = 0; n < len; ++n) {
                        matteValue_t name = matte_bytecode_stub_get_arg_name(stub, n);
                        matte_string_concat_printf(str, " \"");
                        matte_string_concat(str, matte_value_string_get_string_unsafe(vm->store, name));
                        matte_string_concat_printf(str, "\" ");
                    }

                    matte_vm_raise_error_string(vm, str);
                    matte_string_destroy(str); 
                    return matte_store_new_value(vm->store);
                }
            }
        }
        
        int ok = 1;
        if (callable == 2 && len) { // typestrictcheck
            matteArray_t arr = MATTE_ARRAY_CAST(
                ((matteValue_t*)matte_array_get_data(referrables))+1,
                matteValue_t,
                len 
            );
            ok = matte_value_object_function_pre_typecheck_unsafe(vm->store, 
                d, 
                &arr  
            );
        }
        len = matte_bytecode_stub_local_count(stub);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_store_new_value(vm->store);
            matte_array_push(referrables, v);
        }
        

                // no calling context yet
        matteVMStackFrame_t * frame = vm_push_frame(vm);   
        frame->privateBinding = privateBinding; 
       
        if (prettyName) {
            matte_string_set(frame->prettyName, prettyName);
        }

        frame->context = d;
        frame->stub = stub;
        matte_array_at(referrables, matteValue_t, 0) = frame->context;

        // ref copies of values happen here.
        matte_value_object_function_activate_closure(vm->store, frame->context, referrables);

        #ifdef MATTE_DEBUG__STORE
        printf("Context    for frame %d is: %d\n", vm->stacksize, frame->context.value.id);
        {
            uint32_t instcount;
            const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame->stub, &instcount);                    
            matteString_t * info = matte_string_create_from_c_str("REFERRABLE MADE @");
            if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))) {
                matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
            }
            matte_string_destroy(info);
        }
        #endif
        
        

        // establishes the reference path of objects not allowed to be cleaned up
        matte_value_object_push_lock(vm->store, frame->context);        
        matte_value_object_push_lock(vm->store, frame->privateBinding);
        matteValue_t result = matte_store_new_value(vm->store);


        // push locks for all current frame 
        if (prevFrame) {
            len = matte_array_get_size(prevFrame->valueStack);
            for(i = 0; i < len; ++i) {
                matte_value_object_push_lock(vm->store, matte_array_at(prevFrame->valueStack, matteValue_Extended_t, i).value);        
            }
        }

        if (callable == 2) {
            if (ok) 
                result = vm_execution_loop(vm);
            if (!vm->pendingCatchable)            
                matte_value_object_function_post_typecheck_unsafe(vm->store, d, result);
        } else {
            result = vm_execution_loop(vm);        
        }

        matte_value_object_push_lock(vm->store, result);
        matte_store_garbage_collect(vm->store);
        
        if (prevFrame) {
            len = matte_array_get_size(prevFrame->valueStack);
            for(i = 0; i < len; ++i) {
                matte_value_object_pop_lock(vm->store, matte_array_at(prevFrame->valueStack, matteValue_Extended_t, i).value);        
            }
        };
        
        matte_value_object_pop_lock(vm->store, frame->context);

        // cleanup;
        matte_store_recycle(vm->store, frame->context);
        matte_value_object_pop_lock(vm->store, result);
        matte_value_object_pop_lock(vm->store, privateBinding);
        vm_pop_frame(vm);


        

        // uh oh... unhandled errors...
        if (!vm->stacksize && vm->pendingCatchable) {
            if (vm->unhandled) {
                matteValue_t v = vm->catchable;
                if (!vm->pendingCatchableIsError) {
                    v = matte_store_new_value(vm->store);
                    matteString_t * errMessage = matte_string_create_from_c_str("An uncaught message was sent.");
                    matte_value_into_string(vm->store, &v, errMessage);
                    matte_string_destroy(errMessage);
                }


                vm->unhandled(
                    vm,
                    vm->errorLastFile,
                    vm->errorLastLine,
                    v,
                    vm->unhandledData                   
                );           

                if (!vm->pendingCatchableIsError)
                    matte_store_recycle(vm->store, v);     
            }     
            matte_value_object_pop_lock(vm->store, vm->catchable);
            vm->catchable.binIDreserved = 0;
            vm->pendingCatchable = 0;
            vm->pendingCatchableIsError = 0;
            return matte_store_new_value(vm->store);
        }
        return result; // ok, vm_execution_loop returns new
    } 
}


matteValue_t matte_vm_call(
    matteVM_t * vm, 
    matteValue_t func, 
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
) {
    matteValue_t callingContext = {};
    return matte_vm_call_full(vm, func, callingContext, args, argNames, prettyName);
}







matteValue_t matte_vm_run_fileid(
    matteVM_t * vm, 
    uint32_t fileid, 
    matteValue_t parameters
) {
    if (fileid != MATTE_VM_DEBUG_FILE) {
        matteValue_t * precomp = (matteValue_t*)matte_table_find_by_uint(vm->imported, fileid);
        if (precomp) return *precomp;
    }
    matteValue_t func = matte_store_new_value(vm->store);
    matteBytecodeStub_t * stub = vm_find_stub(vm, fileid, 0);
    if (!stub) {
        matte_vm_raise_error_cstring(vm, "Script has no toplevel context to run.");
        return func;
    }
    matte_value_into_new_function_ref(vm->store, &func, stub);
    matte_value_object_push_lock(vm->store, func);


    
    matteValue_t * ref = (matteValue_t*)matte_table_find_by_uint(vm->imported, fileid);
    if (ref) {
        matte_value_object_pop_lock(vm->store, *ref);
    } else {
        ref = (matteValue_t*)matte_allocate(sizeof(matteValue_t));    
        *ref = matte_store_new_value(vm->store);
        matte_table_insert_by_uint(vm->imported, fileid, ref);
    }

    matteArray_t argNames = MATTE_ARRAY_CAST(&vm->specialString_parameters, matteValue_t, 1);
    matteArray_t args = MATTE_ARRAY_CAST(&parameters, matteValue_t, 1);

    matteValue_t result = matte_vm_call(vm, func, &args, &argNames, matte_vm_get_script_name_by_id(vm, fileid));

    *ref = result;
    matte_value_object_push_lock(vm->store, *ref);

    matte_value_object_pop_lock(vm->store, func);
    matte_store_recycle(vm->store, func);
    return result;
}




matteValue_t matte_vm_import(
    matteVM_t * vm, 
    const matteString_t * path, 
    const matteString_t * alias,
    int preloadOnly,
    matteValue_t parameters
) {
    matteValue_t pathStr = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &pathStr, path);

    matteValue_t aliasStr = matte_store_new_value(vm->store);
    if (alias != NULL)
        matte_value_into_string(vm->store, &aliasStr, alias);
    
    matteValue_t preloadBool = matte_store_new_value(vm->store);
    if (preloadOnly)
        matte_value_into_boolean(vm->store, &preloadBool, preloadOnly);
    
    
    matteValue_t args[] = {
        pathStr,
        parameters,
        aliasStr,
        preloadBool
    };
    matte_value_object_push_lock(vm->store, parameters);
    matteValue_t v =  vm_ext_call__import(vm, matte_store_new_value(vm->store), args, NULL);
    matte_value_object_pop_lock(vm->store, parameters);

    matte_store_recycle(vm->store, pathStr);
    matte_store_recycle(vm->store, aliasStr);
    


    if (!vm->stacksize && vm->pendingCatchable) {
        if (vm->unhandled) {
            vm->unhandled(
                vm,
                vm->errorLastFile,
                vm->errorLastLine,
                vm->catchable,
                vm->unhandledData                   
            );                
        }     
        matte_value_object_pop_lock(vm->store, vm->catchable);
        vm->catchable.binIDreserved = 0;
        vm->pendingCatchable = 0;
        vm->pendingCatchableIsError = 0;
        return matte_store_new_value(vm->store);
    }
    return v;
}


static void debug_compile_error(
    const matteString_t * s,
    uint32_t line,
    uint32_t ch,
    void * data
) {
    matteVM_t * vm = (matteVM_t*)data;
    matteValue_t v = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &v, s);
    vm->debug(
        vm,
        MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
        MATTE_VM_DEBUG_FILE,
        line, 
        v,
        vm->debugData
    );
    matte_store_recycle(vm->store, v);
}

matteValue_t matte_vm_run_scoped_debug_source(
    matteVM_t * vm,
    const matteString_t * src,
    int callstackIndex,
    void(*onError)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *),
    void * onErrorData
) {
    matte_store_push_lock_gc(vm->store);
    vm->namedRefIndex = callstackIndex;
    void(*realDebug)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *) = vm->debug;
    void * realDebugData = vm->debugData;
    
    // override current VM stuff
    vm->debug = onError;
    vm->debugData = onErrorData;
    matteValue_t catchable = vm->catchable;
    int pendingCatchable = vm->pendingCatchable;
    int pendingCatchableIsError = vm->pendingCatchableIsError;
    vm->catchable.binIDreserved = 0;
    vm->pendingCatchable = 0;
    vm->pendingCatchableIsError = 0;
 
    uint32_t jitSize = 0;
    uint8_t * jitBuffer = matte_compiler_run_with_named_references(
        matte_get_syntax_graph(vm->matte),
        (uint8_t*)matte_string_get_c_str(src), // TODO: UTF8
        matte_string_get_length(src),
        &jitSize,
        debug_compile_error,
        vm
    ); 
    matteValue_t result;
    if (jitSize) {
        matteArray_t * jitstubs = matte_bytecode_stubs_from_bytecode(
            vm->store, MATTE_VM_DEBUG_FILE, jitBuffer, jitSize
        );
        
        matte_vm_add_stubs(vm, jitstubs);
        result = matte_vm_run_fileid(vm, MATTE_VM_DEBUG_FILE, matte_store_new_value(vm->store));
        matte_array_destroy(jitstubs);
    } else {
        result = matte_store_new_value(vm->store);
    }
    
    // revert vm state 
    vm->debug = realDebug;
    vm->debugData = realDebugData;
    matte_value_object_pop_lock(vm->store, vm->catchable);        
    matte_store_recycle(vm->store, vm->catchable);      
    
      
    vm->pendingCatchable = pendingCatchable;
    vm->catchable = catchable;
    vm->pendingCatchableIsError = pendingCatchableIsError;
    matte_store_pop_lock_gc(vm->store);

    return result;
}


/*
{
    callstack : {
        length => Number,
        frames : [
            {
                file : StringValue,
                lineNumber : Number
            },
        
        ]
    }   
}

*/

matteValue_t vm_info_new_object(matteVM_t * vm, matteValue_t detail) {
    matteValue_t out = matte_store_new_value(vm->store);
    matte_value_into_new_object_ref(vm->store, &out);
    
    matteValue_t callstack = matte_store_new_value(vm->store);
    matte_value_into_new_object_ref(vm->store, &callstack);
    
    matteValue_t key = matte_store_new_value(vm->store);
    matteValue_t val = matte_store_new_value(vm->store);

    uint32_t i;
    uint32_t len = matte_vm_get_stackframe_size(vm);
    matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "length"));
    matte_value_into_number(vm->store, &val, len);
    matte_value_object_set(vm->store, callstack, key, val, 0);
    
    
    matteValue_t * arr = (matteValue_t*)matte_allocate(sizeof(matteValue_t) * len);
    matteString_t * str;
    if (matte_value_type(detail) == MATTE_VALUE_TYPE_STRING) {
        str = matte_string_create_from_c_str("%s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->store, detail)));        
    } else {
        str = matte_string_create_from_c_str("<no string data available>\n");    
    }
    
    for(i = 0; i < len; ++i) {
        matteVMStackFrame_t framesrc = matte_vm_get_stackframe(vm, i);

        matteValue_t frame = matte_store_new_value(vm->store);
        matte_value_into_new_object_ref(vm->store, &frame);

        const matteString_t * filename = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(framesrc.stub));
        
        matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "file"));
        matte_value_into_string(vm->store, &val, filename ? filename : MATTE_VM_STR_CAST(vm, "<unknown>"));        
        matte_value_object_set(vm->store, frame, key, val, 0);
        

        
        
        uint32_t instcount = 0;
        const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(framesrc.stub, &instcount);        
        
        matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "lineNumber"));        
        uint32_t lineNumber = 0;
        if (framesrc.pc - 1 < instcount) {
            lineNumber = inst[framesrc.pc-1].info.lineOffset + matte_bytecode_stub_get_starting_line(framesrc.stub);                
        } 
        matte_value_into_number(vm->store, &val, lineNumber);
        matte_value_object_set(vm->store, frame, key, val, 0);
        arr[i] = frame;        
        
        matte_string_concat_printf(str, " (%d) -> %s, line %d\n", i, filename ? matte_string_get_c_str(filename) : "<unknown>", lineNumber); 
    }

    matteArray_t arrA = MATTE_ARRAY_CAST(arr, matteValue_t, len);
    matte_value_into_new_object_array_ref(vm->store, &val, &arrA);
    matte_deallocate(arr);
    matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "frames"));
    matte_value_object_set(vm->store, callstack, key, val, 0);
    

    
    
    matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "callstack"));    
    matte_value_object_set(vm->store, out, key, callstack, 0);


    matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "summary"));
    matte_value_into_string(vm->store, &val, str);
    matte_string_destroy(str);
    matte_value_object_set(vm->store, out, key, val, 0);

    matte_value_into_string(vm->store, &key, MATTE_VM_STR_CAST(vm, "detail"));
    matte_value_object_set(vm->store, out, key, detail, 0);    

    return out;
}

void matte_vm_print_stack(matteVM_t * vm) {
    matteValue_t a = vm_info_new_object(vm, matte_store_new_value(vm->store));
    printf("%s", matte_string_get_c_str(
        matte_value_string_get_string_unsafe(
            vm->store,
            matte_value_object_access_string(   
                vm->store,
                a,
                MATTE_VM_STR_CAST(vm, "summary")
            )
        )
    ));
}


void matte_vm_raise_error(matteVM_t * vm, matteValue_t val) {
    if (vm->pendingCatchable) {
        return;
    }

    matteValue_t info = vm_info_new_object(vm, val);
    vm->catchable = info;
    matte_value_object_push_lock(vm->store, info);
    if (!vm->stacksize) {
        vm->errorLastFile = 0;
        vm->errorLastLine = -1;
        vm->pendingCatchable = 1;
        vm->pendingCatchableIsError = 1;

        if (vm->debug) {     
            vm->debug(
                vm,
                MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
                0,
                -1,
                val,
                vm->debugData                   
            );
        }
        return;
    }

    uint32_t instcount;
    matteVMStackFrame_t framesrc = matte_vm_get_stackframe(vm, 0);
    const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(framesrc.stub, &instcount);        
    if (framesrc.pc - 1 < instcount) {
        vm->errorLastLine = inst[framesrc.pc-1].info.lineOffset + matte_bytecode_stub_get_starting_line(framesrc.stub);                
    } else {
        vm->errorLastLine = -1;
    }
    vm->errorLastFile = matte_bytecode_stub_get_file_id(framesrc.stub);

    
    
    if (vm->debug) {
        matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, 0);
        uint32_t numinst;
        const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);
        uint32_t line = 0;
        if (frame.pc-1 < numinst && frame.pc-1 >= 0)
            line = inst[frame.pc-1].info.lineOffset + matte_bytecode_stub_get_starting_line(frame.stub);        
        vm->debug(
            vm,
            MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
            matte_bytecode_stub_get_file_id(frame.stub),
            line,
            val,
            vm->debugData                   
        );
    }
    vm->pendingCatchable = 1;
    vm->pendingCatchableIsError = 1;
    
}

void matte_vm_raise_error_string(matteVM_t * vm, const matteString_t * str) {
    matteValue_t b = matte_store_new_value(vm->store);
    matte_value_into_string(vm->store, &b, str);
    matte_vm_raise_error(vm, b);
    matte_store_recycle(vm->store, b);
}

void matte_vm_raise_error_cstring(matteVM_t * vm, const char * str) {
    matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, str));
}

// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i) {
    matteVMStackFrame_t ** frames = (matteVMStackFrame_t**)matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_cstring(vm, "Invalid stackframe requested.");
        matteVMStackFrame_t err = {0};
        return err;
    }
    return *frames[i];
}

uint32_t matte_vm_get_stackframe_size(const matteVM_t * vm) {
    return vm->stacksize;
}

matteValue_t * matte_vm_stackframe_get_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID) {
    matteVMStackFrame_t ** frames = (matteVMStackFrame_t**)matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_cstring(vm, "Invalid stackframe requested in referrable query.");
        return NULL;
    }


    // get context
    if (referrableID < matte_array_get_size(frames[i]->referrables)) {
        return matte_value_object_function_get_closure_value_unsafe(vm->store, frames[i]->context, referrableID);        
    } else {
        matteValue_t * ref = matte_value_get_captured_value(vm->store, frames[i]->context, referrableID - matte_array_get_size(frames[i]->referrables));

        // bad referrable
        if (!ref) {
            matte_vm_raise_error_cstring(vm, "Invalid referrable");
            return NULL;    
        } else { 
            return ref;
        }
    }
}

void matte_vm_stackframe_set_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID, matteValue_t val) {
    matteVMStackFrame_t ** frames = (matteVMStackFrame_t**)matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_cstring(vm, "Invalid stackframe requested in referrable assignment.");
        return;
    }


    // get context
    if (referrableID < matte_array_get_size(frames[i]->referrables)) {
        matte_value_object_function_set_closure_value_unsafe(vm->store, frames[i]->context, referrableID, val);
    } else {
        matte_value_set_captured_value(vm->store, 
            frames[i]->context, 
            referrableID - matte_array_get_size(frames[i]->referrables),
            val
        );
    }
}

void matte_vm_set_external_function_autoname(
    matteVM_t * vm, 
    const matteString_t * identifier,
    uint8_t argCount,
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, const matteValue_t * args, void * userData),
    void * userData
) {
    if (argCount > 26) return;
    matteString_t * arr[argCount+1];
    uint32_t i;
    const char * names[] = {
        "a", "b", "c", "d", "e", "f", "g", "h",
        "i", "j", "k", "l", "m", "n", "o", "p",
        "q", "r", "s", "t", "u", "v", "w", "x",
        "y", "z", ""
    };
    for(i = 0; i < argCount; ++i) {
        arr[i] = (matteString_t*)MATTE_VM_STR_CAST(vm, names[i]);
    }
    matteArray_t conv = MATTE_ARRAY_CAST(arr, matteString_t *, argCount);
    matte_vm_set_external_function(
        vm,
        identifier,
        &conv,
        userFunction,
        userData
    );
}

void matte_vm_set_external_function(
    matteVM_t * vm, 
    const matteString_t * identifier,
    const matteArray_t * argNames,
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, const matteValue_t * args, void * userData),
    void * userData
) {
    if (!userFunction) return;



    uint32_t * id = (uint32_t*)matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        id = (uint32_t*)matte_allocate(sizeof(uint32_t));
        *id = matte_array_get_size(vm->externalFunctionIndex);
        matte_array_set_size(vm->externalFunctionIndex, *id+1);
        matte_table_insert(vm->externalFunctions, identifier, id);
    }

    vm_add_built_in(
        vm,
        *id, 
        argNames, 
        userFunction
    );
    matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, *id).userData = userData;


}

// Gets a new function object that, when called, calls the registered 
// C function.
matteValue_t matte_vm_get_external_function_as_value(
    matteVM_t * vm,
    const matteString_t * identifier
) {
    uint32_t * id = (uint32_t*)matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        return matte_store_new_value(vm->store);
    }

    return matte_array_at(vm->extFuncs, matteValue_t, *id);
}

matteValue_t * matte_vm_get_external_builtin_function_as_value(
    matteVM_t * vm,
    matteExtCall_t ext    
) {
    if (ext > MATTE_EXT_CALL_GETEXTERNALFUNCTION) return NULL;
    return &matte_array_at(vm->extFuncs, matteValue_t, ext);
}


int matte_vm_pending_message(matteVM_t * vm) {
    return vm->pendingCatchable;
}


void matte_vm_set_debug_callback(
    matteVM_t * vm,
    void(*debugCB)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t, void *),
    void * data
) {
    vm->debug = debugCB;
    vm->debugData = data;
}

void matte_vm_set_unhandled_callback(
    matteVM_t * vm,
    void(*cb)(matteVM_t *, uint32_t file, int lineNumber, matteValue_t, void *),
    void * data
) {
    vm->unhandled = cb;
    vm->unhandledData = data;
}

void matte_vm_add_shutdown_callback(
    matteVM_t * vm,
    void(*fn)(matteVM_t *, void *),
    void * data
) {
    MatteCleanupFunctionSet f;
    f.fn = fn;
    f.data = data;
    
    matte_array_push(vm->cleanupFunctionSets, f);
}


void matte_vm_set_print_callback(
    matteVM_t * vm,
    void(*printCB)(matteVM_t *, const matteString_t *, void *),
    void * userData
) {
    vm->userPrint = printCB;
    vm->userPrintData = userData;
}


matteValue_t matte_system_shared__create_memory_buffer_from_raw(matteVM_t * vm, const uint8_t * data, uint32_t size);

matteValue_t matte_vm_create_memory_buffer_handle_from_data(
    matteVM_t * vm,
    const uint8_t * data,
    uint32_t size
) {
    return matte_system_shared__create_memory_buffer_from_raw(
        vm,
        data,
        size
    );
}


uint8_t * matte_system_shared__get_raw_from_memory_buffer(matteVM_t * vm, matteValue_t b, uint32_t * size);

uint8_t * matte_vm_get_memory_buffer_handle_raw_data(
    matteVM_t * vm,
    matteValue_t handle,
    uint32_t * size
) {
    return matte_system_shared__get_raw_from_memory_buffer(
        vm,
        handle,
        size
    );
}
