#include "matte_vm.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_heap.h"
#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_opcode.h"
#include "matte_compiler.h"
#include "./rom/native.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define matte_string_temp_max_calls 128
struct matteVM_t {

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
    matteTable_t * importPath2ID;
    matteTable_t * id2importPath;

    // import for scripts
    uint8_t * (*userImport)(
        matteVM_t *,
        const matteString_t * importPath,
        uint32_t * preexistingFileID,
        uint32_t * dataLength,
        void * usrdata
    );
    void * userImportData;
    matteTable_t * defaultImport_table;
    matteArray_t * importPaths;
    uint32_t nextID;

    int pendingCatchable;
    int pendingCatchableIsError;
    int errorLastLine;
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

};


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
            matteValue_t v = matte_array_at(frame->valueStack, matteValue_t, n);
            if ((v.binID == MATTE_VALUE_TYPE_FUNCTION || v.binID == MATTE_VALUE_TYPE_OBJECT) && v.value.id == id) {
                printf("@ stackframe %d, valuestack %d: %s\n", i, n, v.binID == MATTE_VALUE_TYPE_FUNCTION ? "(function)" : "(object)");
            }
        }
    }
}

#endif



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
    uint8_t * (*import)(
        matteVM_t *,
        const matteString_t * importPath,
        uint32_t * preexistingFileID,
        uint32_t * dataLength,
        void * usrdata
    ),
    void * userData
) {
    vm->userImport = import;
    vm->userImportData = userData;
}

static uint8_t * vm_default_import(
    matteVM_t * vm,
    const matteString_t * importPath,
    uint32_t * fileid,
    uint32_t * dataLength,
    void * usrdata
) {
    uint32_t * id = matte_table_find(vm->defaultImport_table, importPath);
    if (id) {
        *dataLength = 0;
        *fileid = *id;
        return NULL;
    }


    FILE * f = fopen(matte_string_get_c_str(importPath), "rb");
    if (!f) {
        *dataLength = 0;
        return NULL;
    }

    #define DEFAULT_DUMP_SIZE 1024
    char * msg = malloc(DEFAULT_DUMP_SIZE);
    uint32_t totalLen = 0;
    uint32_t len;
    while((len = fread(msg, 1, DEFAULT_DUMP_SIZE, f))) {
        totalLen += len;
    }
    fseek(f, SEEK_SET, 0);
    uint8_t * outBuffer = malloc(totalLen);
    uint8_t * iter = outBuffer;
    while((len = fread(msg, 1, DEFAULT_DUMP_SIZE, f))) {
        memcpy(iter, msg, len);
        iter += len;
    }
    free(msg);
    fclose(f);
    id = malloc(sizeof(uint32_t));
    *id = matte_vm_get_new_file_id(vm, importPath);
    matte_table_insert(vm->defaultImport_table, importPath, id);

    *fileid = *id;
    *dataLength = totalLen;
    return outBuffer;
}

// todo: how to we implement this in a way that we dont "waste"
// IDs by failed /excessive calls to this when the IDs arent used?
uint32_t matte_vm_get_new_file_id(matteVM_t * vm, const matteString_t * name) {
    uint32_t fileid = vm->nextID++; 

    uint32_t * fileidPtr = malloc(sizeof(uint32_t));
    *fileidPtr = fileid;

    matte_table_insert(vm->importPath2ID, name, fileidPtr);   
    matte_table_insert_by_uint(vm->id2importPath, fileid, matte_string_clone(name));

    return fileid;
}

matteImportFunction_t matte_vm_get_default_import() {
    return vm_default_import;
}

uint32_t matte_vm_get_file_id_by_name(matteVM_t * vm, const matteString_t * name) {
    uint32_t * p = matte_table_find(vm->importPath2ID, name);   
    if (!p) return 0xffffffff;
    return *p;
}


typedef struct {
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, const matteValue_t * args, void * userData);
    void * userData;
    uint8_t nArgs;
} ExternalFunctionSet_t;

static matteVMStackFrame_t * vm_push_frame(matteVM_t * vm) {
    vm->stacksize++;
    matteVMStackFrame_t * frame;
    if (matte_array_get_size(vm->callstack) < vm->stacksize) {
        while(matte_array_get_size(vm->callstack) < vm->stacksize) {
            frame = calloc(1, sizeof(matteVMStackFrame_t));
            matte_array_push(vm->callstack, frame);
        }
        frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);

        // initialize
        frame->pc = 0;
        frame->prettyName = matte_string_create();
        frame->context = matte_heap_new_value(vm->heap); // will contain captures
        frame->stub = NULL;
        frame->valueStack = matte_array_create(sizeof(matteValue_t));
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
    matteTable_t * subt = matte_table_find_by_uint(vm->stubIndex, fileid);
    if (!subt) return NULL;
    return matte_table_find_by_uint(subt, stubid);
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
      case MATTE_OPERATOR_SHIFT_LEFT: return vm_operator__overload_only_2(vm, "<<", a, b);
      case MATTE_OPERATOR_SHIFT_RIGHT:return vm_operator__overload_only_2(vm, ">>", a, b);
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

      default:
        matte_vm_raise_error_cstring(vm, "unhandled OPR operator");                        
        return matte_heap_new_value(vm->heap);
    }
}


static matteValue_t vm_operator_1(matteVM_t * vm, matteOperator_t op, matteValue_t a) {
    switch(op) {
      case MATTE_OPERATOR_NOT:         return vm_operator__not(vm, a);
      case MATTE_OPERATOR_NEGATE:      return vm_operator__negate(vm, a);
      case MATTE_OPERATOR_BITWISE_NOT: return vm_operator__bitwise_not(vm, a);
      case MATTE_OPERATOR_POUND:       return vm_operator__overload_only_1(vm, "#", a);
      case MATTE_OPERATOR_TOKEN:       return vm_operator__overload_only_1(vm, "$", a);

      default:
        matte_vm_raise_error_cstring(vm, "unhandled OPR operator");                        
        return matte_heap_new_value(vm->heap);
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
#define STACK_POP() matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1); matte_value_object_pop_lock(vm->heap, matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1)); matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_POP_NORET() matte_value_object_pop_lock(vm->heap, matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1)); matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_PEEK(__n__) (matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1-(__n__)))
#define STACK_PUSH(__v__) matte_array_push(frame->valueStack, __v__); matte_value_object_push_lock(vm->heap, __v__);

static matteValue_t vm_execution_loop(matteVM_t * vm) {
    matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);
    #ifdef MATTE_DEBUG__VM
        const matteString_t * str = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub));
    #endif
    const matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    uint32_t sfscount = 0;
    const matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);

  RELOOP:
    while(frame->pc < instCount) {
        inst = program+frame->pc++;
        // TODO: optimize out
        #ifdef MATTE_DEBUG__VM
            if (matte_array_get_size(frame->valueStack))
                matte_value_print(vm->heap, matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1));
            printf("from %s, line %d, CALLSTACK%6d PC%6d, OPCODE %s, Stacklen: %10d\n", str ? matte_string_get_c_str(str) : "???", inst->lineNumber, vm->stacksize, frame->pc, opcode_to_str(inst->opcode), matte_array_get_size(frame->valueStack));
            fflush(stdout);
        #endif
        if (vm->debug) {
            if (vm->lastLine != inst->lineNumber) {
                matteValue_t db = matte_heap_new_value(vm->heap);
                vm->debug(vm, MATTE_VM_DEBUG_EVENT__LINE_CHANGE, matte_bytecode_stub_get_file_id(frame->stub), inst->lineNumber, db, vm->debugData);
                vm->lastLine = inst->lineNumber;
                matte_heap_recycle(vm->heap, db);
            }
        }


        switch(inst->opcode) {
          case MATTE_OPCODE_NOP:
            break;
            
          case MATTE_OPCODE_PRF: {
            uint32_t referrable;
            memcpy(&referrable, inst->data, sizeof(uint32_t));
            matteValue_t * v = matte_vm_stackframe_get_referrable(vm, 0, referrable);
            if (v) {
                matteValue_t copy = matte_heap_new_value(vm->heap);
                matte_value_into_copy(vm->heap, &copy, *v);
                STACK_PUSH(copy);
            }
            break;
          }
          
          case MATTE_OPCODE_PNR: {
            uint32_t referrableStrID;
            memcpy(&referrableStrID, inst->data, sizeof(uint32_t));
            matteValue_t v = matte_bytecode_stub_get_string(frame->stub, referrableStrID);
            if (!v.binID) {
                matte_vm_raise_error_cstring(vm, "No such bytecode stub string.");
            } else {
                matteVMStackFrame_t f = matte_vm_get_stackframe(vm, vm->namedRefIndex+1);
                if (f.context.binID) {
                    matteValue_t v0 = matte_value_frame_get_named_referrable(vm->heap, 
                        &f, 
                        v
                    ); 
                    STACK_PUSH(v0);
                }
            }
            break;
          }

          case MATTE_OPCODE_NEM: {
            matteValue_t v = matte_heap_new_value(vm->heap);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_NNM: {
            double val;
            memcpy(&val, inst->data, sizeof(double));
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_number(vm->heap, &v, val);
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NBL: {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_boolean(vm->heap, &v, inst->data[0]);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_NST: {
            uint32_t stringID;
            memcpy(&stringID, inst->data, sizeof(uint32_t));

            matteValue_t v = matte_bytecode_stub_get_string(frame->stub, stringID);
            if (!v.binID) {
                matte_vm_raise_error_cstring(vm, "NST opcode refers to non-existent string (corrupt bytecode?)");
                break;
            }
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NOB: {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_new_object_ref(vm->heap, &v);
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_neutral(vm->heap, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
            #endif
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_SFS:
            memcpy(&sfscount, inst->data, 4);

            if (frame->pc >= instCount) {
                break;
            }
            inst = program+frame->pc++;

            // FALLTHROUGH PURPOSEFULLY
            
          
          
          case MATTE_OPCODE_NFN: {
            uint32_t ids[2];
            memcpy(ids, inst->data, 8);

            matteBytecodeStub_t * stub = vm_find_stub(vm, ids[0], ids[1]);

            if (stub == NULL) {
                matte_vm_raise_error_cstring(vm, "NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)");
                break;
            }

            if (sfscount) {
                matteValue_t v = matte_heap_new_value(vm->heap);
                uint32_t i;
                if (STACK_SIZE() < sfscount) {
                    matte_vm_raise_error_cstring(vm, "VM internal error: too few values on stack to service SFS opcode!");
                    break;
                }
                matteValue_t * vals = calloc(sfscount, sizeof(matteValue_t));
                // reverse order since on the stack is [retval] [arg n-1] [arg n-2]...
                for(i = 0; i < sfscount; ++i) {
                    vals[sfscount - i - 1] = STACK_PEEK(i);
                }

                // xfer ownership of type values
                matteArray_t arr = MATTE_ARRAY_CAST(vals, matteValue_t, sfscount);
                matte_value_into_new_typed_function_ref(vm->heap, &v, stub, &arr);
                #ifdef MATTE_DEBUG__HEAP
                    matte_heap_track_neutral(vm->heap, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
                #endif

                free(vals);

                for(i = 0; i < sfscount; ++i) {
                    STACK_POP_NORET();
                }
                sfscount = 0;
                STACK_PUSH(v);
                
            } else {
                matteValue_t v = matte_heap_new_value(vm->heap);
                matte_value_into_new_function_ref(vm->heap, &v, stub);
                #ifdef MATTE_DEBUG__HEAP
                    matte_heap_track_neutral(vm->heap, v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
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
            matte_value_object_insert(
                vm->heap,
                obj,
                matte_value_object_get_number_key_count(vm->heap, STACK_PEEK(1)),
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
                vm->heap,
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
             
            matteValue_t p = STACK_POP();
            matteValue_t target = STACK_PEEK(0);
            matte_value_object_push_lock(vm->heap, p);
                       
            uint32_t len = matte_value_object_get_number_key_count(vm->heap, p);
            uint32_t i;
            uint32_t keylen = matte_value_object_get_number_key_count(vm->heap, target);
            for(i = 0; i < len; ++i) {
                matte_value_object_insert(
                    vm->heap,
                    target, 
                    keylen++,
                    matte_value_object_access_index(vm->heap, p, i)
                );
            }
            matte_value_object_pop_lock(vm->heap, p);
            break;            
          }
  

          case MATTE_OPCODE_SPO: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare key-value pairs for object construction, but there are an odd number of items on the stack.");    
                break;
            }
             
            matteValue_t p = STACK_POP();
            matte_value_object_push_lock(vm->heap, p);
            matteValue_t keys = matte_value_object_keys(vm->heap, p);
            matte_value_object_push_lock(vm->heap, keys);
            matteValue_t vals = matte_value_object_values(vm->heap, p);
            matte_value_object_push_lock(vm->heap, vals);

            matteValue_t target = STACK_PEEK(0);
                       
            uint32_t len = matte_value_object_get_number_key_count(vm->heap, keys);
            uint32_t i;
            matteValue_t item;            
            for(i = 0; i < len; ++i) {
                matte_value_object_set(
                    vm->heap,
                    target,
                    matte_value_object_access_index(vm->heap, keys, i),
                    matte_value_object_access_index(vm->heap, vals, i),
                    1
                );
            }
            
            matte_value_object_pop_lock(vm->heap, keys);
            matte_value_object_pop_lock(vm->heap, vals);
            matte_value_object_pop_lock(vm->heap, p);
            break;            
          }
          
          
          case MATTE_OPCODE_PTO: {
            uint32_t typecode;
            memcpy(&typecode, inst->data, sizeof(uint32_t));            
            matteValue_t v;
            
            switch(typecode) {
              case 0: v = *matte_heap_get_empty_type(vm->heap); break;           
              case 1: v = *matte_heap_get_boolean_type(vm->heap); break;           
              case 2: v = *matte_heap_get_number_type(vm->heap); break;           
              case 3: v = *matte_heap_get_string_type(vm->heap); break;           
              case 4: v = *matte_heap_get_object_type(vm->heap); break;           
              case 5: v = *matte_heap_get_function_type(vm->heap); break;           
              case 6: v = *matte_heap_get_type_type(vm->heap); break;           
              case 7: v = *matte_heap_get_any_type(vm->heap); break;           
                
            }
            STACK_PUSH(v);
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
            matteValue_t key = STACK_PEEK(0);
            if (stackSize > 2 && key.binID == MATTE_VALUE_TYPE_STRING) {
                while(i < stackSize && key.binID == MATTE_VALUE_TYPE_STRING) {
                    matte_array_push(argnames, key);
                    i++;
                    key = STACK_PEEK(i);
                    matte_array_push(args, key);                
                    i++;
                    key = STACK_PEEK(i);
                }
            }
            uint32_t argcount = matte_array_get_size(args);
            
            if (i == stackSize) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for a call, but insufficient arguments on the stack.");    
                matte_array_destroy(args);            
                matte_array_destroy(argnames);            
                break;
            }

            matteValue_t function = STACK_PEEK(i);

            #ifdef MATTE_DEBUG__HEAP
                matteString_t * info = matte_string_create_from_c_str("FUNCTION CALLED @");
                if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)))
                    matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
                matte_heap_track_neutral(vm->heap, function, matte_string_get_c_str(info), inst->lineNumber);
                matte_string_destroy(info);
            #endif

            matteValue_t result = matte_vm_call(vm, function, args, argnames, NULL);

            for(i = 0; i < argcount; ++i) {
                STACK_POP_NORET();
                matteValue_t v = matte_array_at(args, matteValue_t, i);
                matte_heap_recycle(vm->heap, v);
                STACK_POP_NORET(); // always a string
            }
            matte_array_destroy(args);
            matte_array_destroy(argnames);
            STACK_POP_NORET();
            matte_heap_recycle(vm->heap, function);
            STACK_PUSH(result);
            break;
          }
          case MATTE_OPCODE_ARF: {            
            if (STACK_SIZE() < 1) {
                matte_vm_raise_error_cstring(vm, "VM error: tried to prepare arguments for referrable assignment, but insufficient arguments on the stack.");    
                break;            
            }
            matteValue_t * ref = matte_vm_stackframe_get_referrable(vm, 0, *(uint32_t*)inst->data); 
            if (ref) {
                matteValue_t v = STACK_PEEK(0);
                matteValue_t vOut;
                switch(*(uint32_t*)(inst->data+4) + (int)MATTE_OPERATOR_ASSIGNMENT_NONE) {
                  case MATTE_OPERATOR_ASSIGNMENT_NONE: {
                    matte_vm_stackframe_set_referrable(vm, 0, *(uint32_t*)inst->data, v);
                    vOut = v;
                    v.binID = 0;
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
                    vOut = matte_heap_new_value(vm->heap);
                    matte_vm_raise_error_cstring(vm, "VM error: tried to access non-existent referrable operation (corrupt bytecode?).");                        

                }                
                STACK_POP_NORET();
                matte_heap_recycle(vm->heap, v);
                STACK_PUSH(vOut); // new value is pushed
            } else {
                matte_vm_raise_error_cstring(vm, "VM error: tried to access non-existent referrable.");    
            }
            break;
          }          
          case MATTE_OPCODE_POP: {
            uint32_t popCount = *(uint32_t *)inst->data;
            while (popCount && STACK_SIZE()) {
                matteValue_t m = STACK_POP();
                matte_heap_recycle(vm->heap, m);
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
            matteValue_t cpy = matte_heap_new_value(vm->heap);
            matte_value_into_copy(vm->heap, &cpy, m);
            STACK_PUSH(cpy);
            break;
          }   
          case MATTE_OPCODE_OSN: {
            if (STACK_SIZE() < 3) {
                matte_vm_raise_error_cstring(vm, "VM error: OSN opcode requires 3 on the stack.");                
                break;        
            }
            int opr = *(uint32_t*)(inst->data);
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
                
                const matteValue_t * lk = matte_value_object_set(vm->heap, object, key, val, isBracket);
                matteValue_t lknp = matte_heap_new_value(vm->heap);
                if (lk) {
                    matte_value_into_copy(vm->heap, &lknp, *lk);
                }
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_PUSH(lknp);

            
            } else {
                matteValue_t * ref = matte_value_object_access_direct(vm->heap, object, key, isBracket);
                matteValue_t refH = {};
                int isDirect = 1;
                if (!ref) {
                    isDirect = 0;
                    refH = matte_value_object_access(vm->heap, object, key, isBracket);
                    ref = &refH;
                }
                matte_value_object_push_lock(vm->heap, *ref);
                matteValue_t out = matte_heap_new_value(vm->heap);
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
                
                matte_value_object_pop_lock(vm->heap, *ref);
                // Slower path for things like accessors
                // Indirect access means the ref being worked with is essentially a copy, so 
                // we need to set the object value back after the operator has been applied.
                if (!isDirect) {
                    matte_value_object_set(
                        vm->heap,
                        object,
                        key, 
                        refH,
                        isBracket
                    );
                }
                if (refH.binID)     
                    matte_heap_recycle(vm->heap, refH); 
                    
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_POP_NORET();
                STACK_PUSH(out);
            }



            matte_heap_recycle(vm->heap, key);
            matte_heap_recycle(vm->heap, object);  
            matte_heap_recycle(vm->heap, val);

            break;
          }    

          case MATTE_OPCODE_OLK: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_cstring(vm, "VM error: OLK opcode requires 2 on the stack.");                
                break;        
            }
            
            uint32_t isBracket = *(uint32_t*)inst->data;
            matteValue_t key = STACK_PEEK(0);
            matteValue_t object = STACK_PEEK(1);            
            matteValue_t output = matte_value_object_access(vm->heap, object, key, isBracket);
            
            STACK_POP_NORET();
            STACK_POP_NORET();
            matte_heap_recycle(vm->heap, key);
            matte_heap_recycle(vm->heap, object);            
            STACK_PUSH(output);
            break;
          }    


          case MATTE_OPCODE_EXT: {
            uint64_t call = *(uint64_t*)inst->data;
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
            uint32_t count = *(uint32_t*)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (!matte_value_as_boolean(vm->heap, condition)) {
                frame->pc += count;
            }
            STACK_POP_NORET();
            matte_heap_recycle(vm->heap, condition);
            break;
          }
          case MATTE_OPCODE_SCA: {
            uint32_t count = *(uint32_t*)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (!matte_value_as_boolean(vm->heap, condition)) {
                frame->pc += count;
            }
            break;
          }
          case MATTE_OPCODE_SCO: {
            uint32_t count = *(uint32_t*)inst->data;
            matteValue_t condition = STACK_PEEK(0);
            if (matte_value_as_boolean(vm->heap, condition)) {
                frame->pc += count;
            }
            break;
          }

          case MATTE_OPCODE_ASP: {
            uint32_t count = *(uint32_t*)inst->data;
            frame->pc += count;
            break;
          }  
          
          case MATTE_OPCODE_QRY: {
            matteValue_t o = STACK_PEEK(0);
            matteValue_t output = matte_value_query(vm->heap, &o, *(uint32_t*)inst->data);
            STACK_POP_NORET();            

            STACK_PUSH(output);
            // re-insert the base as "base"
            if (output.binID == MATTE_VALUE_TYPE_FUNCTION) {
                STACK_PUSH(o);
                STACK_PUSH(vm->specialString_base);
            } 
            
            break;
          }
          
          
          case MATTE_OPCODE_OPR: {            
            switch(inst->data[0]) {
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
                case MATTE_OPERATOR_NOTEQ: {
                    if (STACK_SIZE() < 2) {
                        matte_vm_raise_error_cstring(vm, "OPR operator requires 2 operands.");                        
                    } else {
                        matteValue_t a = STACK_PEEK(1);
                        matteValue_t b = STACK_PEEK(0);
                        matteValue_t v = vm_operator_2(
                            vm,
                            inst->data[0],
                            a, b
                        );
                        STACK_POP_NORET();
                        STACK_POP_NORET();
                        matte_heap_recycle(vm->heap, a);
                        matte_heap_recycle(vm->heap, b);
                        STACK_PUSH(v); // ok
                    }
                    break;                
                }
                    
                
                case MATTE_OPERATOR_NOT:
                case MATTE_OPERATOR_NEGATE:
                case MATTE_OPERATOR_BITWISE_NOT:
                case MATTE_OPERATOR_POUND:
                case MATTE_OPERATOR_TOKEN:{
                    if (STACK_SIZE() < 1) {
                        matte_vm_raise_error_cstring(vm, "OPR operator requires 1 operand.");                        
                    } else {
                    
                        matteValue_t a = STACK_PEEK(0);
                        matteValue_t v = vm_operator_1(
                            vm,
                            inst->data[0],
                            a
                        );
                        STACK_POP_NORET();
                        matte_heap_recycle(vm->heap, a);
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

    // it's VERY important that the restart condition is not run when 
    // pendingCatchable is true, as the stack value top does not correspond
    // to anything meaningful
    if (frame->restartCondition && !vm->pendingCatchable) {
        matteValue_t v = (matte_array_get_size(frame->valueStack) ? matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1) : matte_heap_new_value(vm->heap));
        if (frame->restartCondition(vm, frame, v, frame->restartConditionData)) {
            frame->pc = 0;
            goto RELOOP;
        }
    }

    // top of stack is output
    if (matte_array_get_size(frame->valueStack)) {
        
        matteValue_t output = matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1);
        uint32_t i;
        uint32_t len = matte_array_get_size(frame->valueStack);
        for(i = 0; i < len-1; ++i) { 
            matte_value_object_pop_lock(vm->heap, matte_array_at(frame->valueStack, matteValue_t, i));
            matte_heap_recycle(vm->heap, matte_array_at(frame->valueStack, matteValue_t, i));
        }
        matte_value_object_pop_lock(vm->heap, matte_array_at(frame->valueStack, matteValue_t, len-1));
        matte_array_set_size(frame->valueStack, 0);
        return output; // ok since not removed from normal value stack stuff
    } else {
        return matte_heap_new_value(vm->heap);
    }



}

#define WRITE_BYTES(__T__, __VAL__) matte_array_push_n(arr, &(__VAL__), sizeof(__T__));
#define WRITE_NBYTES(__VAL__, __N__) matte_array_push_n(arr, &(__VAL__), __N__);

static void write_unistring(matteArray_t * arr, matteString_t * str) {
    uint32_t len = matte_string_get_length(str);
    WRITE_BYTES(uint32_t, len);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        int32_t ch = matte_string_get_char(str, i);
        WRITE_BYTES(int32_t, ch);
    }
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
    
    u8 = matte_array_get_size(argNames);
    WRITE_BYTES(uint8_t, u8); 
    uint32_t i;
    for(i = 0; i < u8; ++i) {
        write_unistring(arr, matte_array_at(argNames, matteString_t *, i));
    }
    
    
    matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
        vm->heap,
        0,
        matte_array_get_data(arr), 
        matte_array_get_size(arr)
    );
    matte_array_destroy(arr);    
    matteBytecodeStub_t * out = matte_array_at(stubs, matteBytecodeStub_t *, 0);

    matte_array_destroy(stubs);
    matte_array_at(vm->extStubs, matteBytecodeStub_t *, index) = out;

    matteValue_t func = {};
    matte_value_into_new_function_ref(vm->heap, &func, out);
    matte_value_object_push_lock(vm->heap, func);
    matte_array_at(vm->extFuncs, matteValue_t, index) = func;
}

#include "MATTE_EXT_NUMBER"
#include "MATTE_EXT_OBJECT"
#include "MATTE_EXT_STRING"

matteVM_t * matte_vm_create() {
    matteVM_t * vm = calloc(1, sizeof(matteVM_t));


    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t *));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    vm->externalFunctions = matte_table_create_hash_matte_string();
    vm->externalFunctionIndex = matte_array_create(sizeof(ExternalFunctionSet_t));
    vm->heap = matte_heap_create(vm);

    vm->extStubs = matte_array_create(sizeof(matteBytecodeStub_t *));
    vm->extFuncs = matte_array_create(sizeof(matteValue_t));
    vm->importPath2ID = matte_table_create_hash_matte_string();
    vm->id2importPath = matte_table_create_hash_pointer();
    vm->imported = matte_table_create_hash_pointer();
    vm->userImport = vm_default_import;
    vm->defaultImport_table = matte_table_create_hash_matte_string();
    vm->importPaths = matte_array_create(sizeof(matteString_t *));
    vm->nextID = 1;
    vm->cleanupFunctionSets = matte_array_create(sizeof(MatteCleanupFunctionSet));
    
    vm->specialString_from = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_from, MATTE_VM_STR_CAST(vm, "from"));
    vm->specialString_message = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_message, MATTE_VM_STR_CAST(vm, "message"));
    vm->specialString_parameters = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_parameters, MATTE_VM_STR_CAST(vm, "parameters"));
    vm->specialString_base = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_base, MATTE_VM_STR_CAST(vm, "base"));
    
    // add built in functions
    const matteString_t * forever_name = MATTE_VM_STR_CAST(vm, "do");
    const matteString_t * query_name = MATTE_VM_STR_CAST(vm, "base");
    const matteString_t * for_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "do")
    };
    const matteString_t * import_names[] = {
        MATTE_VM_STR_CAST(vm, "module"),
        MATTE_VM_STR_CAST(vm, "parameters")
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
        MATTE_VM_STR_CAST(vm, "inherits")
    };
    const matteString_t * of = MATTE_VM_STR_CAST(vm, "of");
    const matteString_t * setAttributes_names[] = {
        query_name,
        MATTE_VM_STR_CAST(vm, "attributes")
    };
    const matteString_t * type = MATTE_VM_STR_CAST(vm, "type");
    const matteString_t * message = MATTE_VM_STR_CAST(vm, "message");
    const matteString_t * detail = MATTE_VM_STR_CAST(vm, "detail");
    const matteString_t * listen_names[] = {
        MATTE_VM_STR_CAST(vm, "to"),
        MATTE_VM_STR_CAST(vm, "onMessage"),
        MATTE_VM_STR_CAST(vm, "onError")
    };
    const matteString_t * name = MATTE_VM_STR_CAST(vm, "name");
    const matteString_t * value = MATTE_VM_STR_CAST(vm, "value");
    vm->specialString_value = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_value, value);
    const matteString_t * previous = MATTE_VM_STR_CAST(vm, "previous");
    vm->specialString_previous = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &vm->specialString_previous, previous);

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

    
    const matteString_t * findIndex_names[] = {
        query_name,
        value,
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
    temp = MATTE_ARRAY_CAST(&forever_name, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_FOREVER,   &temp, vm_ext_call__forever);
    temp = MATTE_ARRAY_CAST(&import_names, matteString_t *, 2);vm_add_built_in(vm, MATTE_EXT_CALL_IMPORT,  &temp, vm_ext_call__import);

    temp = MATTE_ARRAY_CAST(&message, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_PRINT,      &temp, vm_ext_call__print);
    temp = MATTE_ARRAY_CAST(&message, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_SEND,       &temp, vm_ext_call__send);
    temp = MATTE_ARRAY_CAST(listen_names, matteString_t *, 3);vm_add_built_in(vm, MATTE_EXT_CALL_LISTEN,     &temp, vm_ext_call__listen);
    temp = MATTE_ARRAY_CAST(&detail, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_ERROR,      &temp, vm_ext_call__error);

    temp = MATTE_ARRAY_CAST(&name, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL_GETEXTERNALFUNCTION, &temp, vm_ext_call__getexternalfunction);



    // NUMBERS
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__PI,        &temp, vm_ext_call__number__pi);    
    temp = MATTE_ARRAY_CAST(&stringName, matteString_t *, 1);vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__PARSE,     &temp, vm_ext_call__number__parse);    
    temp = *emptyArr;                                   vm_add_built_in(vm, MATTE_EXT_CALL__NUMBER__RANDOM,    &temp, vm_ext_call__number__random);    


    // STRING
    temp = MATTE_ARRAY_CAST(&strings, matteString_t *, 1);   vm_add_built_in(vm, MATTE_EXT_CALL__STRING__COMBINE,     &temp, vm_ext_call__string__combine);    



    // OBJECTS
    temp = MATTE_ARRAY_CAST(type_names, matteString_t *, 2);  vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__NEWTYPE,     &temp, vm_ext_call__object__newtype);    
    temp = MATTE_ARRAY_CAST(&type, matteString_t *, 1);   vm_add_built_in(vm, MATTE_EXT_CALL__OBJECT__INSTANTIATE,     &temp, vm_ext_call__object__instantiate);    

    
    
    
    
    

    // QUERY: NUMBER
    temp = MATTE_ARRAY_CAST(atan2Names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ATAN2,       &temp, vm_ext_call__number__atan2);    

    // QUERY: OBJECT 
    temp = MATTE_ARRAY_CAST(push_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__PUSH,     &temp, vm_ext_call__object__push);    
    temp = MATTE_ARRAY_CAST(push_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__INSERT,     &temp, vm_ext_call__object__insert);    
    temp = MATTE_ARRAY_CAST(removeKey_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REMOVE,     &temp, vm_ext_call__object__remove);    
    temp = MATTE_ARRAY_CAST(setAttributes_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SETATTRIBUTES,     &temp, vm_ext_call__object__set_attributes);    
    temp = MATTE_ARRAY_CAST(sort_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SORT,     &temp, vm_ext_call__object__sort);    
    temp = MATTE_ARRAY_CAST(subset_names, matteString_t *, 3);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SUBSET,     &temp, vm_ext_call__object__subset);    
    temp = MATTE_ARRAY_CAST(filterNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FILTER,     &temp, vm_ext_call__object__filter);    
    temp = MATTE_ARRAY_CAST(findIndex_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FINDINDEX,     &temp, vm_ext_call__object__findindex);    
    temp = MATTE_ARRAY_CAST(is_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ISA,     &temp, vm_ext_call__object__is);    
    temp = MATTE_ARRAY_CAST(mapReduceNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__MAP,     &temp, vm_ext_call__object__map);    
    temp = MATTE_ARRAY_CAST(mapReduceNames, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__REDUCE,     &temp, vm_ext_call__object__reduce);    
    temp = MATTE_ARRAY_CAST(conditional_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ANY,     &temp, vm_ext_call__object__any);    
    temp = MATTE_ARRAY_CAST(conditional_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__ALL,     &temp, vm_ext_call__object__all);    
    temp = MATTE_ARRAY_CAST(for_names, matteString_t *, 2);vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FOR,    &temp, vm_ext_call__object__for);
    temp = MATTE_ARRAY_CAST(for_names, matteString_t *, 2);vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__FOREACH, &temp, vm_ext_call__object__foreach);

    
    
    // QUERY: STRING 
    
    temp = MATTE_ARRAY_CAST(search_names, matteString_t *, 2);   vm_add_built_in(vm, MATTE_EXT_CALL__QUERY__SEARCH,     &temp, vm_ext_call__string__search);    
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

    
    
    



    //vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_NOWRITE, 0, vm_ext_call__introspect_nowrite);    
    matte_bind_native_functions(vm);

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
    
    

    matteTableIter_t * iter = matte_table_iter_create();
    matteTableIter_t * subiter = matte_table_iter_create();

    for(matte_table_iter_start(iter, vm->imported);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matteValue_t * v = matte_table_iter_get_value(iter);
        matte_value_object_pop_lock(vm->heap, *v);
        matte_heap_recycle(vm->heap, *v);
        free(v);
    }

    len = matte_array_get_size(vm->extFuncs);
    for(i = 0; i < len; ++i) {
        matte_value_object_pop_lock(vm->heap, matte_array_at(vm->extFuncs, matteValue_t, i));
    }
    matte_array_destroy(vm->extFuncs);



    matte_table_destroy(vm->imported);
    matte_heap_destroy(vm->heap);
    

    

    for(matte_table_iter_start(iter, vm->stubIndex);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matteTable_t * table = matte_table_iter_get_value(iter);

        for(matte_table_iter_start(subiter, table);
            !matte_table_iter_is_end(subiter);
            matte_table_iter_proceed(subiter)) {
            matte_bytecode_stub_destroy((void*)matte_table_iter_get_value(subiter));
        }

        matte_table_destroy(table);
    }
    matte_table_destroy(vm->stubIndex);

    for(matte_table_iter_start(iter, vm->defaultImport_table);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        free(matte_table_iter_get_value(iter));
    }
    matte_table_destroy(vm->defaultImport_table);

    len = matte_array_get_size(vm->importPaths);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(vm->importPaths, matteString_t *, i));
    }
    matte_array_destroy(vm->importPaths);


    for(matte_table_iter_start(iter, vm->id2importPath);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matte_string_destroy(matte_table_iter_get_value(iter));
    }
    matte_table_destroy(vm->id2importPath);

    for(matte_table_iter_start(iter, vm->importPath2ID);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        free(matte_table_iter_get_value(iter));
    }
    matte_table_destroy(vm->importPath2ID);


    for(matte_table_iter_start(iter, vm->externalFunctions);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        free(matte_table_iter_get_value(iter));
    }
    
    matte_table_destroy(vm->externalFunctions);




    len = matte_array_get_size(vm->callstack);
    for(i = 0; i < len; ++i) {
        matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t *, i);
        matte_string_destroy(frame->prettyName);
        matte_array_destroy(frame->valueStack);
        free(frame);
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
    free(vm);
}

const matteString_t * matte_vm_get_script_name_by_id(matteVM_t * vm, uint32_t fileid) {
    return matte_table_find_by_uint(vm->id2importPath, fileid);
}

void matte_vm_add_stubs(matteVM_t * vm, const matteArray_t * arr) {
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    for(i = 0; i < len; ++i) {
        matteBytecodeStub_t * stub = matte_array_at(arr, matteBytecodeStub_t *, i);
        matteTable_t * fileindex = matte_table_find_by_uint(vm->stubIndex, matte_bytecode_stub_get_file_id(stub));
        if (!fileindex) {
            fileindex = matte_table_create_hash_pointer();
            matte_table_insert_by_uint(vm->stubIndex, matte_bytecode_stub_get_file_id(stub), fileindex);
        }
        
        matte_table_insert_by_uint(fileindex, matte_bytecode_stub_get_id(stub), stub);
    }
}

matteHeap_t * matte_vm_get_heap(matteVM_t * vm) {return vm->heap;}


matteValue_t matte_vm_call(
    matteVM_t * vm, 
    matteValue_t func, 
    const matteArray_t * args,
    const matteArray_t * argNames,
    const matteString_t * prettyName
) {
    if (vm->pendingCatchable) return matte_heap_new_value(vm->heap);
    int callable = matte_value_is_callable(vm->heap, func); 
    if (!callable) {
        matteString_t * err = matte_string_create_from_c_str("Error: cannot call non-function value ");
        if (prettyName)
            matte_string_concat(err, prettyName);
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_heap_new_value(vm->heap);
    }

    if (matte_array_get_size(args) != matte_array_get_size(argNames)) {
        matte_vm_raise_error_cstring(vm, "VM call as mismatching arguments and parameter names.");            
        return matte_heap_new_value(vm->heap);
    }

    // special case: type object as a function
    if (func.binID == MATTE_VALUE_TYPE_TYPE) {                
        if (matte_array_get_size(args)) {
            if (matte_array_at(argNames, matteValue_t, 0).value.id != vm->specialString_from.value.id) {
                matte_vm_raise_error_cstring(vm, "Type conversion failed: unbound parameter to function ('from')");            
            }
            return matte_value_to_type(vm->heap, matte_array_at(args, matteValue_t, 0), func);
        } else {
            matte_vm_raise_error_cstring(vm, "Type conversion failed (no value given to convert).");
            return matte_heap_new_value(vm->heap);
        }
    }


    // normal function
    matteValue_t d = matte_heap_new_value(vm->heap);
    matte_value_into_copy(vm->heap, &d, func);


    if (matte_bytecode_stub_get_file_id(matte_value_get_bytecode_stub(vm->heap, d)) == 0) {
        // fileid 0 is a special fileid that never refers to a real file.
        // this is used to call external c functions. stubID refers to 
        // which index within externalFunction.
        // In this case, a new stackframe is NOT pushed.
        uint32_t external = matte_bytecode_stub_get_id(matte_value_get_bytecode_stub(vm->heap, d));
        if (external >= matte_array_get_size(vm->externalFunctionIndex)) {
            matte_heap_recycle(vm->heap, d);
            return matte_heap_new_value(vm->heap);            
        }
        ExternalFunctionSet_t * set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, external);
        matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->heap, func);
        matteArray_t * argsReal = matte_array_create(sizeof(matteValue_t));
        uint32_t i, n;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(stub);
        matte_array_set_size(argsReal, len);
        // empty for any unset values.
        memset(matte_array_get_data(argsReal), 0, sizeof(matteValue_t)*len);
        
        for(i = 0; i < lenReal; ++i) {
            for(n = 0; n < len; ++n) {
                if (matte_bytecode_stub_get_arg_name(stub, n).value.id == matte_array_at(argNames, matteValue_t, i).value.id) {
                    matteValue_t v = matte_array_at(args, matteValue_t, i);
                    matte_array_at(argsReal, matteValue_t, n) = v;
                    // sicne this function doesn't use a referrable, we need to set roots manually.
                    if (v.binID == MATTE_VALUE_TYPE_OBJECT || v.binID == MATTE_VALUE_TYPE_FUNCTION) {
                        matte_value_object_push_lock(vm->heap, v);
                    }

                    break;
                }
            }       
            
            if (n == len) {
                matteString_t * str;
                // couldnt find the requested name. Throw an error.
                if (len)
                    str = matte_string_create_from_c_str(
                        "Could not bind requested parameter: '%s'.\n Bindable parameters for this function: ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->heap, matte_array_at(argNames, matteValue_t, i)))
                    );
                else {
                    str = matte_string_create_from_c_str(
                        "Could not bind requested parameter: '%s'.\n (no bindable parameters for this function) ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->heap, matte_array_at(argNames, matteValue_t, i)))
                    );                
                }

                for(n = 0; n < len; ++n) {
                    matteValue_t name = matte_bytecode_stub_get_arg_name(stub, n);
                    matte_string_concat_printf(str, " \"");
                    matte_string_concat(str, matte_value_string_get_string_unsafe(vm->heap, name));
                    matte_string_concat_printf(str, "\" ");
                }
                
                matte_vm_raise_error_string(vm, str);
                matte_string_destroy(str);
                
                matte_array_destroy(argsReal);
                return matte_heap_new_value(vm->heap);
                
            }
        }
        
        
        
        matte_value_object_push_lock(vm->heap, d);
        matteValue_t result = {};
        if (callable == 2) {
            int ok = matte_value_object_function_pre_typecheck_unsafe(vm->heap, d, argsReal);
            if (ok) 
                result = set->userFunction(vm, d, matte_array_get_data(argsReal), set->userData);
            matte_value_object_function_post_typecheck_unsafe(vm->heap, d, result);
        } else {
            result = set->userFunction(vm, d, matte_array_get_data(argsReal), set->userData);        
        }
        matte_value_object_pop_lock(vm->heap, d);


        len = matte_array_get_size(argsReal);
        for(i = 0; i < len; ++i) {
            int bid = matte_array_at(argsReal, matteValue_t, i).binID;
            if (bid == MATTE_VALUE_TYPE_OBJECT ||
                bid == MATTE_VALUE_TYPE_FUNCTION) {
                matte_value_object_pop_lock(vm->heap, matte_array_at(argsReal, matteValue_t, i));
            }
            matte_heap_recycle(vm->heap, matte_array_at(argsReal, matteValue_t, i));
        }
        matte_array_destroy(argsReal);
        matte_heap_recycle(vm->heap, d);
        return result;
    } else {

        matteArray_t * referrables = matte_array_create(sizeof(matteValue_t));
        matteBytecodeStub_t * stub = matte_value_get_bytecode_stub(vm->heap, d);

        // slot 0 is always the context
        uint32_t i, n;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(stub);
        matte_array_set_size(referrables, 1+len);
        // empty for any unset values.
        memset(matte_array_get_data(referrables), 0, sizeof(matteValue_t)*(len+1));

        
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
                        "Could not bind requested parameter: '%s'.\n Bindable parameters for this function: ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->heap, matte_array_at(argNames, matteValue_t, i)))
                    );
                else {
                    str = matte_string_create_from_c_str(
                        "Could not bind requested parameter: '%s'.\n (no bindable parameters for this function) ", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->heap, matte_array_at(argNames, matteValue_t, i)))
                    );                
                }
                
                
                for(n = 0; n < len; ++n) {
                    matteValue_t name = matte_bytecode_stub_get_arg_name(stub, n);
                    matte_string_concat_printf(str, " \"");
                    matte_string_concat(str, matte_value_string_get_string_unsafe(vm->heap, name));
                    matte_string_concat_printf(str, "\" ");
                }

                matte_vm_raise_error_string(vm, str);
                matte_string_destroy(str); 
                
                matte_array_destroy(referrables);
                return matte_heap_new_value(vm->heap);
            }
        }

        int ok = 1;
        if (callable == 2 && len) { // typestrictcheck
            matteArray_t arr = MATTE_ARRAY_CAST(
                ((matteValue_t*)matte_array_get_data(referrables))+1,
                matteValue_t,
                len 
            );
            ok = matte_value_object_function_pre_typecheck_unsafe(vm->heap, 
                d, 
                &arr  
            );
        }
        len = matte_bytecode_stub_local_count(stub);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_array_push(referrables, v);
        }
        

                // no calling context yet
        matteVMStackFrame_t * frame = vm_push_frame(vm);
        if (prettyName) {
            matte_string_set(frame->prettyName, prettyName);
        }

        frame->context = d;
        frame->stub = stub;
        frame->referrable = matte_heap_new_value(vm->heap);
        matte_array_at(referrables, matteValue_t, 0) = frame->context;

        // ref copies of values happen here.
        matte_value_into_new_object_array_ref(vm->heap, &frame->referrable, referrables);

        #ifdef MATTE_DEBUG__HEAP
        printf("Context    for frame %d is: %d\n", vm->stacksize, frame->context.value.id);
        printf("Referrable for frame %d is: %d\n", vm->stacksize, frame->referrable.value.id);
        {
            uint32_t instcount;
            const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame->stub, &instcount);                    
            matteString_t * info = matte_string_create_from_c_str("REFERRABLE MADE @");
            if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))) {
                matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
            }
            matte_heap_track_neutral(vm->heap, frame->referrable, matte_string_get_c_str(info), inst->lineNumber);
            matte_string_destroy(info);
        }
        #endif
        
        
        frame->referrableSize = matte_array_get_size(referrables);
        matte_array_destroy(referrables);

        // establishes the reference path of objects not allowed to be cleaned up
        matte_value_object_push_lock(vm->heap, frame->referrable);
        matte_value_object_push_lock(vm->heap, frame->context);        
        matteValue_t result = matte_heap_new_value(vm->heap);
        if (callable == 2) {
            if (ok) 
                result = vm_execution_loop(vm);
            if (!vm->pendingCatchable)            
                matte_value_object_function_post_typecheck_unsafe(vm->heap, d, result);
        } else {
            result = vm_execution_loop(vm);        
        }
        matte_value_object_pop_lock(vm->heap, frame->referrable);
        matte_value_object_pop_lock(vm->heap, frame->context);

        // cleanup;
        matte_heap_recycle(vm->heap, frame->context);
        matte_heap_recycle(vm->heap, frame->referrable);
        matte_value_object_push_lock(vm->heap, result);
        matte_heap_garbage_collect(vm->heap);
        matte_value_object_pop_lock(vm->heap, result);
        vm_pop_frame(vm);


        

        // uh oh... unhandled errors...
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
            matte_value_object_pop_lock(vm->heap, vm->catchable);
            vm->catchable.binID = 0;
            vm->pendingCatchable = 0;
            vm->pendingCatchableIsError = 0;
            return matte_heap_new_value(vm->heap);
        }
        return result; // ok, vm_execution_loop returns new
    } 
}

matteValue_t matte_vm_run_fileid(
    matteVM_t * vm, 
    uint32_t fileid, 
    matteValue_t parameters,
    const matteString_t * importPath
) {
    if (fileid != MATTE_VM_DEBUG_FILE) {
        matteValue_t * precomp = matte_table_find_by_uint(vm->imported, fileid);
        if (precomp) return *precomp;
    }
    matteValue_t func = matte_heap_new_value(vm->heap);
    matteBytecodeStub_t * stub = vm_find_stub(vm, fileid, 0);
    if (!stub) {
        matte_vm_raise_error_cstring(vm, "Script has no toplevel context to run.");
        return func;
    }
    matte_value_into_new_function_ref(vm->heap, &func, stub);
    matte_value_object_push_lock(vm->heap, func);


    matteArray_t argNames = MATTE_ARRAY_CAST(&vm->specialString_parameters, matteValue_t, 1);
    matteArray_t args = MATTE_ARRAY_CAST(&parameters, matteValue_t, 1);

    matteString_t * locationExpanded;
    if (importPath) {
        locationExpanded = import__push_path(vm->importPaths, importPath);
    }
    matteValue_t result = matte_vm_call(vm, func, &args, &argNames, matte_vm_get_script_name_by_id(vm, fileid));
    if (importPath) {
        matte_string_destroy(locationExpanded);
        import__pop_path(vm->importPaths);
    }

    matteValue_t * ref = malloc(sizeof(matteValue_t));
    *ref = result;
    matte_table_insert_by_uint(vm->imported, fileid, ref);
    matte_value_object_push_lock(vm->heap, *ref);

    matte_value_object_pop_lock(vm->heap, func);
    matte_heap_recycle(vm->heap, func);
    return result;
}

matteString_t * matte_vm_import_expand_path(
    matteVM_t * vm, 
    const matteString_t * module
) {
    matteString_t * locationExpanded = import__push_path(vm->importPaths, module);
    import__pop_path(vm->importPaths);
    return locationExpanded;
}

const matteString_t * matte_vm_get_import_path(matteVM_t * vm) {
    matteArray_t * paths = vm->importPaths;
    if (matte_array_get_size(paths)) {
        return matte_array_at(paths, matteString_t *, matte_array_get_size(paths)-1);
    } else {
        return NULL;
    }

}

matteValue_t matte_vm_import(
    matteVM_t * vm, 
    const matteString_t * path, 
    matteValue_t parameters
) {
    matteValue_t pathStr = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &pathStr, path);
    matteValue_t args[] = {
        pathStr,
        parameters
    };
    matte_value_object_push_lock(vm->heap, parameters);
    matteValue_t v =  vm_ext_call__import(vm, matte_heap_new_value(vm->heap), args, NULL);
    matte_value_object_pop_lock(vm->heap, parameters);


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
        matte_value_object_pop_lock(vm->heap, vm->catchable);
        vm->catchable.binID = 0;
        vm->pendingCatchable = 0;
        vm->pendingCatchableIsError = 0;
        return matte_heap_new_value(vm->heap);
    }
    return v;
}


static void debug_compile_error(
    const matteString_t * s,
    uint32_t line,
    uint32_t ch,
    void * data
) {
    matteVM_t * vm = data;
    matteValue_t v = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &v, s);
    vm->debug(
        vm,
        MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
        MATTE_VM_DEBUG_FILE,
        line, 
        v,
        vm->debugData
    );
    matte_heap_recycle(vm->heap, v);
}

matteValue_t matte_vm_run_scoped_debug_source(
    matteVM_t * vm,
    const matteString_t * src,
    int callstackIndex,
    void(*onError)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *),
    void * onErrorData
) {
    matte_heap_push_lock_gc(vm->heap);
    vm->namedRefIndex = callstackIndex;
    void(*realDebug)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t value, void *) = vm->debug;
    void * realDebugData = vm->debugData;
    
    // override current VM stuff
    vm->debug = onError;
    vm->debugData = onErrorData;
    matteValue_t catchable = vm->catchable;
    int pendingCatchable = vm->pendingCatchable;
    int pendingCatchableIsError = vm->pendingCatchableIsError;
    vm->catchable.binID = 0;
    vm->pendingCatchable = 0;
    vm->pendingCatchableIsError = 0;
 
    uint32_t jitSize = 0;
    uint8_t * jitBuffer = matte_compiler_run_with_named_references(
        (uint8_t*)matte_string_get_c_str(src), // TODO: UTF8
        matte_string_get_length(src),
        &jitSize,
        debug_compile_error,
        vm
    ); 
    matteValue_t result;
    if (jitSize) {
        matteArray_t * jitstubs = matte_bytecode_stubs_from_bytecode(
            vm->heap, MATTE_VM_DEBUG_FILE, jitBuffer, jitSize
        );
        
        matte_vm_add_stubs(vm, jitstubs);
        result = matte_vm_run_fileid(vm, MATTE_VM_DEBUG_FILE, matte_heap_new_value(vm->heap), NULL);

    } else {
        result = matte_heap_new_value(vm->heap);
    }
    
    // revert vm state 
    vm->debug = realDebug;
    vm->debugData = realDebugData;
    matte_value_object_pop_lock(vm->heap, vm->catchable);        
    matte_heap_recycle(vm->heap, vm->catchable);      
    
      
    vm->pendingCatchable = pendingCatchable;
    vm->catchable = catchable;
    vm->pendingCatchableIsError = pendingCatchableIsError;
    matte_heap_pop_lock_gc(vm->heap);

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
    matteValue_t out = matte_heap_new_value(vm->heap);
    matte_value_into_new_object_ref(vm->heap, &out);
    
    matteValue_t callstack = matte_heap_new_value(vm->heap);
    matte_value_into_new_object_ref(vm->heap, &callstack);
    
    matteValue_t key = matte_heap_new_value(vm->heap);
    matteValue_t val = matte_heap_new_value(vm->heap);

    uint32_t i;
    uint32_t len = matte_vm_get_stackframe_size(vm);
    matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "length"));
    matte_value_into_number(vm->heap, &val, len);
    matte_value_object_set(vm->heap, callstack, key, val, 0);
    
    
    matteValue_t * arr = malloc(sizeof(matteValue_t) * len);
    matteString_t * str;
    if (detail.binID == MATTE_VALUE_TYPE_STRING) {
        str = matte_string_create_from_c_str("%s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(vm->heap, detail)));        
    } else {
        str = matte_string_create_from_c_str("<no string data available>\n");    
    }
    
    for(i = 0; i < len; ++i) {
        matteVMStackFrame_t framesrc = matte_vm_get_stackframe(vm, i);

        matteValue_t frame = matte_heap_new_value(vm->heap);
        matte_value_into_new_object_ref(vm->heap, &frame);

        const matteString_t * filename = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(framesrc.stub));
        
        matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "file"));
        matte_value_into_string(vm->heap, &val, filename ? filename : MATTE_VM_STR_CAST(vm, "<unknown>"));        
        matte_value_object_set(vm->heap, frame, key, val, 0);
        

        
        
        uint32_t instcount = 0;
        const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(framesrc.stub, &instcount);        
        
        matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "lineNumber"));        
        int lineNumber = 0;
        if (framesrc.pc - 1 < instcount) {
            lineNumber = inst[framesrc.pc-1].lineNumber;                
        } 
        matte_value_into_number(vm->heap, &val, lineNumber);
        matte_value_object_set(vm->heap, frame, key, val, 0);
        arr[i] = frame;        
        
        matte_string_concat_printf(str, " (%d) -> %s, line %d\n", i, filename ? matte_string_get_c_str(filename) : "<unknown>", lineNumber); 
    }

    matteArray_t arrA = MATTE_ARRAY_CAST(arr, matteValue_t, len);
    matte_value_into_new_object_array_ref(vm->heap, &val, &arrA);
    free(arr);
    matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "frames"));
    matte_value_object_set(vm->heap, callstack, key, val, 0);
    

    
    
    matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "callstack"));    
    matte_value_object_set(vm->heap, out, key, callstack, 0);


    matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "summary"));
    matte_value_into_string(vm->heap, &val, str);
    matte_string_destroy(str);
    matte_value_object_set(vm->heap, out, key, val, 0);

    matte_value_into_string(vm->heap, &key, MATTE_VM_STR_CAST(vm, "detail"));
    matte_value_object_set(vm->heap, out, key, detail, 0);    

    matte_heap_recycle(vm->heap, val);
    matte_heap_recycle(vm->heap, key);
    matte_heap_recycle(vm->heap, callstack);

    return out;
}


void matte_vm_raise_error(matteVM_t * vm, matteValue_t val) {
    if (vm->pendingCatchable) {
        return;
    }

    matteValue_t info = vm_info_new_object(vm, val);
    vm->catchable = info;
    vm->pendingCatchable = 1;
    vm->pendingCatchableIsError = 1;
    if (!vm->stacksize) {
        vm->errorLastFile = 0;
        vm->errorLastLine = -1;
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
        vm->errorLastLine = inst[framesrc.pc-1].lineNumber;                
    } else {
        vm->errorLastLine = -1;
    }
    vm->errorLastFile = matte_bytecode_stub_get_file_id(framesrc.stub);

    
    matte_value_object_push_lock(vm->heap, info);
    
    if (vm->debug) {
        matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, 0);
        uint32_t numinst;
        const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);
        uint32_t line = 0;
        if (frame.pc-1 < numinst && frame.pc-1 >= 0)
            line = inst[frame.pc-1].lineNumber;        
        vm->debug(
            vm,
            MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
            matte_bytecode_stub_get_file_id(frame.stub),
            line,
            val,
            vm->debugData                   
        );
    }
}

void matte_vm_raise_error_string(matteVM_t * vm, const matteString_t * str) {
    matteValue_t b = matte_heap_new_value(vm->heap);
    matte_value_into_string(vm->heap, &b, str);
    matte_vm_raise_error(vm, b);
    matte_heap_recycle(vm->heap, b);
}

void matte_vm_raise_error_cstring(matteVM_t * vm, const char * str) {
    matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, str));
}

// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i) {
    matteVMStackFrame_t ** frames = matte_array_get_data(vm->callstack);
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
    matteVMStackFrame_t ** frames = matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_cstring(vm, "Invalid stackframe requested in referrable query.");
        return NULL;
    }


    // get context
    if (referrableID < frames[i]->referrableSize) {
        return matte_value_object_array_at_unsafe(vm->heap, frames[i]->referrable, referrableID);        
    } else {
        matteValue_t * ref = matte_value_get_captured_value(vm->heap, frames[i]->context, referrableID - frames[i]->referrableSize);

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
    matteVMStackFrame_t ** frames = matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_cstring(vm, "Invalid stackframe requested in referrable assignment.");
        return;
    }


    // get context
    if (referrableID < frames[i]->referrableSize) {
        matteValue_t vkey = matte_heap_new_value(vm->heap);
        matte_value_into_number(vm->heap, &vkey, referrableID);
        matte_value_object_set(vm->heap, frames[i]->referrable, vkey, val, 1);
        matte_heap_recycle(vm->heap, vkey);
        //matte_value_into_copy(matte_value_object_array_at_unsafe(frames[i]->referrable, referrableID), val);
    } else {
        matte_value_set_captured_value(vm->heap, 
            frames[i]->context, 
            referrableID - frames[i]->referrableSize,
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
    matteString_t * arr[argCount];
    uint32_t i;
    char * names[] = {
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



    uint32_t * id = matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        id = malloc(sizeof(uint32_t));
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
    uint32_t * id = matte_table_find(vm->externalFunctions, identifier);
    if (!id) {
        return matte_heap_new_value(vm->heap);
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
