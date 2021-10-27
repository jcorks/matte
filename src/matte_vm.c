#include "matte_vm.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_heap.h"
#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_opcode.h"
#include "matte_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

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


    void(*debug)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t, void *);
    void * debugData;
   
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
    uint32_t nextID;

    int pendingCatchable;

    // array of ready introspection objects.
    // when introspection objects are destroyed,
    // they are placed in the table before
    matteValue_t introspectObject;

    void * (*userPrint)(
        matteVM_t * vm,
        const matteString_t *,
        void * userData
    );
    void * userPrintData;


};


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
    while(len = fread(msg, 1, DEFAULT_DUMP_SIZE, f)) {
        totalLen += len;
    }
    fseek(f, SEEK_SET, 0);
    uint8_t * outBuffer = malloc(totalLen);
    uint8_t * iter = outBuffer;
    while(len = fread(msg, 1, DEFAULT_DUMP_SIZE, f)) {
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



typedef struct {
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, matteArray_t * args, void * userData);
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
      case MATTE_OPERATOR_SHIFT_LEFT: return vm_operator__shift_left  (vm, a, b);
      case MATTE_OPERATOR_SHIFT_RIGHT:return vm_operator__shift_right (vm, a, b);
      case MATTE_OPERATOR_POW:        return vm_operator__pow         (vm, a, b);
      case MATTE_OPERATOR_EQ:         return vm_operator__eq          (vm, a, b);
      case MATTE_OPERATOR_POINT:      return vm_operator__point       (vm, a, b);
      case MATTE_OPERATOR_TERNARY:    return vm_operator__ternary     (vm, a, b);
      case MATTE_OPERATOR_LESS:       return vm_operator__less        (vm, a, b);
      case MATTE_OPERATOR_GREATER:    return vm_operator__greater     (vm, a, b);
      case MATTE_OPERATOR_LESSEQ:     return vm_operator__lesseq      (vm, a, b);
      case MATTE_OPERATOR_GREATEREQ:  return vm_operator__greatereq   (vm, a, b);
      case MATTE_OPERATOR_TRANSFORM:  return vm_operator__transform   (vm, a, b);
      case MATTE_OPERATOR_NOTEQ:      return vm_operator__noteq       (vm, a, b);
      case MATTE_OPERATOR_MODULO:     return vm_operator__modulo      (vm, a, b);
      case MATTE_OPERATOR_CARET:      return vm_operator__caret       (vm, a, b);

      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled OPR operator"));                        
        return matte_heap_new_value(vm->heap);
    }
}


static matteValue_t vm_operator_1(matteVM_t * vm, matteOperator_t op, matteValue_t a) {
    switch(op) {
      case MATTE_OPERATOR_NOT:         return vm_operator__not(vm, a);
      case MATTE_OPERATOR_NEGATE:      return vm_operator__negate(vm, a);
      case MATTE_OPERATOR_BITWISE_NOT: return vm_operator__bitwise_not(vm, a);
      case MATTE_OPERATOR_POUND:       return vm_operator__pound(vm, a);
      case MATTE_OPERATOR_TOKEN:       return vm_operator__token(vm, a);

      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled OPR operator"));                        
        return matte_heap_new_value(vm->heap);
    }
}





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

      default:
        return "???";
    }
}

#define STACK_SIZE() matte_array_get_size(frame->valueStack)
#define STACK_POP() matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1); matte_value_object_pop_lock(matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1)); matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_PEEK(__n__) (matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1-(__n__)))
#define STACK_PUSH(__v__) matte_array_push(frame->valueStack, __v__); matte_value_object_push_lock(__v__);

static matteValue_t vm_execution_loop(matteVM_t * vm) {
    matteVMStackFrame_t * frame = matte_array_at(vm->callstack, matteVMStackFrame_t*, vm->stacksize-1);
    #ifdef MATTE_DEBUG__VM
        const matteString_t * str = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub));
    #endif
    const matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    uint32_t sfscount = 0;
    const matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);
    matteValue_t output = matte_heap_new_value(vm->heap);


    while(frame->pc < instCount) {
        inst = program+frame->pc++;
        // TODO: optimize out
        #ifdef MATTE_DEBUG__VM
            if (matte_array_get_size(frame->valueStack))
                matte_value_print(matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1));
            printf("from %s, line %d, CALLSTACK%6d PC%6d, OPCODE %s, Stacklen: %10d\n", str ? matte_string_get_c_str(str) : "???", inst->lineNumber, vm->stacksize, frame->pc, opcode_to_str(inst->opcode), matte_array_get_size(frame->valueStack));
            fflush(stdout);
        #endif
        if (vm->debug) {
            if (vm->lastLine != inst->lineNumber) {
                matteValue_t db = matte_heap_new_value(vm->heap);
                vm->debug(vm, MATTE_VM_DEBUG_EVENT__LINE_CHANGE, matte_bytecode_stub_get_file_id(frame->stub), inst->lineNumber, db, vm->debugData);
                vm->lastLine = inst->lineNumber;
                matte_heap_recycle(db);
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
                matte_value_into_copy(&copy, *v);
                STACK_PUSH(copy);
            }
            break;
          }
          
          case MATTE_OPCODE_PNR: {
            uint32_t referrableStrID;
            memcpy(&referrableStrID, inst->data, sizeof(uint32_t));
            const matteString_t * str = matte_bytecode_stub_get_string(frame->stub, referrableStrID);
            if (!str) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("No such bytecode stub string."));
            } else {
                matteVMStackFrame_t f = matte_vm_get_stackframe(vm, vm->namedRefIndex+1);
                if (f.context.binID) {
                    matteValue_t v = matte_value_frame_get_named_referrable(
                        &f, 
                        str
                    ); 
                    STACK_PUSH(v);
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
            matte_value_into_number(&v, val);
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NBL: {
            double val;
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_boolean(&v, inst->data[0]);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_NST: {
            uint32_t stringID;
            memcpy(&stringID, inst->data, sizeof(uint32_t));

            const matteString_t * str = matte_bytecode_stub_get_string(frame->stub, stringID);
            if (!str) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("NST opcode refers to non-existent string (corrupt bytecode?)"));
                break;
            }
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_string(&v, str);
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NOB: {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_new_object_ref(&v);
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
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
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)"));
                break;
            }

            if (sfscount) {
                matteValue_t v = matte_heap_new_value(vm->heap);
                uint32_t i;
                if (STACK_SIZE() < sfscount) {
                    matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM internal error: too few values on stack to service SFS opcode!"));
                    break;
                }
                matteValue_t * vals = calloc(sfscount, sizeof(matteValue_t));
                // reverse order since on the stack is [retval] [arg n-1] [arg n-2]...
                for(i = 0; i < sfscount; ++i) {
                    vals[sfscount - i - 1] = STACK_PEEK(i);
                }

                // xfer ownership of type values
                matte_value_into_new_typed_function_ref(&v, stub, MATTE_ARRAY_CAST(vals, matteValue_t, sfscount));
                #ifdef MATTE_DEBUG__HEAP
                    matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
                #endif

                free(vals);

                for(i = 0; i < sfscount; ++i) {
                    STACK_POP();
                }
                sfscount = 0;
                STACK_PUSH(v);
                
            } else {
                matteValue_t v = matte_heap_new_value(vm->heap);
                matte_value_into_new_function_ref(&v, stub);
                #ifdef MATTE_DEBUG__HEAP
                    matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
                #endif

                STACK_PUSH(v);
            }
            break;
          }
          case MATTE_OPCODE_NAR: {

              
            uint32_t len;
            memcpy(&len, inst->data, 4);
            if (STACK_SIZE() < len) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("Array cannot be created. (insufficient stack size)"));
                break;
            }
            matteValue_t v = matte_heap_new_value(vm->heap);
            uint32_t i;
            matteArray_t * args = matte_array_create(sizeof(matteValue_t));
            matte_array_set_size(args, len);
            matteValue_t t;
            for(i = 0; i < len; ++i) {
                matte_array_at(args, matteValue_t, len-i-1) = STACK_PEEK(i);
            }

            matte_value_into_new_object_array_ref(&v, args);
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
            #endif

            for(i = 0; i < len; ++i) {
                STACK_POP();
                matte_heap_recycle(matte_array_at(args, matteValue_t, i));
            }
            matte_array_destroy(args);

            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NSO: {
            uint32_t len;
            memcpy(&len, inst->data, 4);
            if (STACK_SIZE() < len*2) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("Static object cannot be created. (insufficient stack size)"));
                break;
            }

            matteValue_t v = matte_heap_new_value(vm->heap);
            uint32_t i;
            matteArray_t * args = matte_array_create(sizeof(matteValue_t));

            matteValue_t va, ke;
            for(i = 0; i < len; ++i) {
                va = STACK_PEEK(i*2+0);
                ke = STACK_PEEK(i*2+1);
                matte_array_push(args, ke);  
                matte_array_push(args, va);  
            }

            matte_value_into_new_object_literal_ref(&v, args);
            #ifdef MATTE_DEBUG__HEAP
                matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
            #endif


            len = matte_array_get_size(args);
            for(i = 0; i < len; ++i) {
                STACK_POP();
                matte_heap_recycle(matte_array_at(args, matteValue_t, i));
            }
            matte_array_destroy(args);

            STACK_PUSH(v);
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
              case 5: v = *matte_heap_get_type_type(vm->heap); break;           
              case 6: v = *matte_heap_get_any_type(vm->heap); break;           
                
            }
            STACK_PUSH(v);
            break;
          }
          
  
          case MATTE_OPCODE_CAL: {

            uint32_t argcount;
            memcpy(&argcount, inst->data, sizeof(uint32_t));
            if (STACK_SIZE() < argcount+1) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack."));    
                break;
            }



            matteArray_t * args = matte_array_create(sizeof(matteValue_t));
            matte_array_set_size(args, argcount);

            uint32_t i;
            for(i = 0; i < argcount; ++i) {
                matteValue_t v = STACK_PEEK(i);
                matte_array_at(args, matteValue_t, argcount-i-1) = v;
            }

            matteValue_t function = STACK_PEEK(argcount);

            #ifdef MATTE_DEBUG__HEAP
                matteString_t * info = matte_string_create_from_c_str("FUNCTION CALLED @");
                if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)))
                    matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
                matte_heap_track_neutral(function, matte_string_get_c_str(info), inst->lineNumber);
                matte_string_destroy(info);
            #endif

            matteValue_t result = matte_vm_call(vm, function, args, NULL);

            for(i = 0; i < argcount; ++i) {
                STACK_POP();
                matteValue_t v = matte_array_at(args, matteValue_t, i);
                matte_heap_recycle(v);
            }
            matte_array_destroy(args);
            STACK_POP();
            matte_heap_recycle(function);
            STACK_PUSH(result);
            break;
          }
          case MATTE_OPCODE_ARF: {            
            if (STACK_SIZE() < 1) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to prepare arguments for referrable assignment, but insufficient arguments on the stack."));    
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
                    matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to access non-existent referrable operation (corrupt bytecode?)."));                        

                }                
                STACK_POP();
                matte_heap_recycle(v);
                STACK_PUSH(vOut); // new value is pushed
            } else {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to access non-existent referrable."));    
            }
            break;
          }          
          case MATTE_OPCODE_POP: {
            uint32_t popCount = *(uint32_t *)inst->data;
            while (popCount && STACK_SIZE()) {
                matteValue_t m = STACK_POP();
                matte_heap_recycle(m);
                popCount--;
            }
            break;
          }    
          case MATTE_OPCODE_CPY: {
            if (!STACK_SIZE()) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: cannot CPY with empty stack"));    
                break;
            }       
            matteValue_t m = STACK_PEEK(0);
            matteValue_t cpy = matte_heap_new_value(vm->heap);
            matte_value_into_copy(&cpy, m);
            STACK_PUSH(cpy);
            break;
          }   
          case MATTE_OPCODE_OSN: {
            if (STACK_SIZE() < 3) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: OSN opcode requires 3 on the stack."));                
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
                
                const matteValue_t * lk = matte_value_object_set(object, key, val, isBracket);
                matteValue_t lknp = matte_heap_new_value(vm->heap);
                if (lk) {
                    matte_value_into_copy(&lknp, *lk);
                }
                STACK_POP();
                STACK_POP();
                STACK_POP();
                STACK_PUSH(lknp);

            
            } else {
                matteValue_t * ref = matte_value_object_access_direct(object, key, isBracket);
                matteValue_t refH = {};
                if (!ref) {
                    refH = matte_value_object_access(object, key, isBracket);
                    ref = &refH;
                }
                matte_value_object_push_lock(*ref);
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
                    matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to access non-existent assignment operation (corrupt bytecode?)."));                        
                }               
                
                matte_value_object_pop_lock(*ref);
                if (refH.binID)     
                    matte_heap_recycle(refH); 
                    
                STACK_POP();
                STACK_POP();
                STACK_POP();
                STACK_PUSH(out);
            }



            matte_heap_recycle(key);
            matte_heap_recycle(object);  
            matte_heap_recycle(val);

            break;
          }    

          case MATTE_OPCODE_OLK: {
            if (STACK_SIZE() < 2) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: OLK opcode requires 2 on the stack."));                
                break;        
            }
            
            uint32_t isBracket = *(uint32_t*)inst->data;
            matteValue_t key = STACK_PEEK(0);
            matteValue_t object = STACK_PEEK(1);            
            matteValue_t output = matte_value_object_access(object, key, isBracket);
            
            STACK_POP();
            STACK_POP();
            matte_heap_recycle(key);
            matte_heap_recycle(object);            
            STACK_PUSH(output);
            break;
          }    


          case MATTE_OPCODE_EXT: {
            uint64_t call = *(uint64_t*)inst->data;
            if (call >= matte_array_get_size(vm->externalFunctionIndex)) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: unknown external call."));                
                break;        
            }
            matteBytecodeStub_t * stub = matte_array_at(vm->extStubs, matteBytecodeStub_t *, call);
            if (stub == NULL) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("EXT opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)"));
                break;
            }

            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_new_function_ref(&v, stub);
            #ifdef MATTE_DEBUG__HEAP
                if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)))
                    matte_heap_track_neutral(v, matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))), inst->lineNumber);
            #endif
            STACK_PUSH(v);
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
            if (!matte_value_as_boolean(condition)) {
                frame->pc += count;
            }
            STACK_POP();
            matte_heap_recycle(condition);
            break;
          }    
          case MATTE_OPCODE_ASP: {
            uint32_t count = *(uint32_t*)inst->data;
            frame->pc += count;
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
                        matte_vm_raise_error_string(vm, MATTE_STR_CAST("OPR operator requires 2 operands."));                        
                    } else {
                        matteValue_t a = STACK_PEEK(0);
                        matteValue_t b = STACK_PEEK(1);
                        matteValue_t v = vm_operator_2(
                            vm,
                            inst->data[0],
                            a, b
                        );
                        STACK_POP();
                        STACK_POP();
                        matte_heap_recycle(a);
                        matte_heap_recycle(b);
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
                        matte_vm_raise_error_string(vm, MATTE_STR_CAST("OPR operator requires 1 operand."));                        
                    } else {
                    
                        matteValue_t a = STACK_PEEK(0);
                        matteValue_t v = vm_operator_1(
                            vm,
                            inst->data[0],
                            a
                        );
                        STACK_POP();
                        matte_heap_recycle(a);
                        STACK_PUSH(v);
                    }
                    break;                
                }
            }
            break;
          }
      
          default: 
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("Unknown / unhandled opcode.")); 
        
        }


        // catchables are checked after every instruction.
        // once encountered they are handled immediately.
        // If there is no handler, the catchable propogates down the callstack.
        if (vm->pendingCatchable) {
            break;
        }
        
    }
    // top of stack is output
    if (matte_array_get_size(frame->valueStack)) {
        
        matteValue_t output = matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1);
        uint32_t i;
        uint32_t len = matte_array_get_size(frame->valueStack);
        for(i = 0; i < len-1; ++i) { 
            matte_value_object_pop_lock(matte_array_at(frame->valueStack, matteValue_t, i));
            matte_heap_recycle(matte_array_at(frame->valueStack, matteValue_t, i));
        }
        matte_value_object_pop_lock(matte_array_at(frame->valueStack, matteValue_t, len-1));
        matte_array_set_size(frame->valueStack, 0);
        return output; // ok since not removed from normal value stack stuff
    } else {
        return matte_heap_new_value(vm->heap);
    }



}


static void vm_add_built_in(
    matteVM_t * vm,
    uint32_t index, 
    int nArgs, 
    matteValue_t (*cb)(matteVM_t *, matteValue_t v, matteArray_t * args, void * userData)
) {
    ExternalFunctionSet_t * set;
    if (matte_array_get_size(vm->externalFunctionIndex) <= index) {
        matte_array_set_size(vm->externalFunctionIndex, index+1);
    }
    if (matte_array_get_size(vm->extStubs) <= index) {
        matte_array_set_size(vm->extStubs, index+1);
    }

    set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, index);
    set->userFunction = cb;
    set->userData = NULL;
    set->nArgs = nArgs;

    

    typedef struct {
        uint8_t bytes[1+4+1];
    } FakeID;
    FakeID id = {0};
    uint8_t nArgsU = nArgs;
    id.bytes[0] = 1;
    memcpy(id.bytes+1+0, &index,  sizeof(uint32_t));
    memcpy(id.bytes+1+4, &nArgsU, sizeof(uint8_t));

    matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
        0,
        id.bytes, 
        6
    );
    
    matteBytecodeStub_t * out = matte_array_at(stubs, matteBytecodeStub_t *, 0);

    matte_array_destroy(stubs);
    matte_array_at(vm->extStubs, matteBytecodeStub_t *, index) = out;
}

#include "MATTE_INTROSPECT"

matteVM_t * matte_vm_create() {
    matteVM_t * vm = calloc(1, sizeof(matteVM_t));
    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t *));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    vm->externalFunctions = matte_table_create_hash_matte_string();
    vm->externalFunctionIndex = matte_array_create(sizeof(ExternalFunctionSet_t));
    vm->heap = matte_heap_create(vm);

    vm->extStubs = matte_array_create(sizeof(matteBytecodeStub_t *));
    vm->importPath2ID = matte_table_create_hash_matte_string();
    vm->id2importPath = matte_table_create_hash_pointer();
    vm->imported = matte_table_create_hash_pointer();
    vm->userImport = vm_default_import;
    vm->defaultImport_table = matte_table_create_hash_matte_string();
    vm->nextID = 1;

    // add built in functions
    vm_add_built_in(vm, MATTE_EXT_CALL_NOOP,    0, vm_ext_call__noop);
    vm_add_built_in(vm, MATTE_EXT_CALL_LOOP,    1, vm_ext_call__loop);
    vm_add_built_in(vm, MATTE_EXT_CALL_FOR,     2, vm_ext_call__for);
    vm_add_built_in(vm, MATTE_EXT_CALL_FOREACH, 2, vm_ext_call__foreach);
    vm_add_built_in(vm, MATTE_EXT_CALL_MATCH,   2, vm_ext_call__match);
    vm_add_built_in(vm, MATTE_EXT_CALL_IMPORT,  2, vm_ext_call__import);
    vm_add_built_in(vm, MATTE_EXT_CALL_REMOVE_KEY,  2, vm_ext_call__remove_key);
    vm_add_built_in(vm, MATTE_EXT_CALL_SET_OPERATOR,  2, vm_ext_call__set_operator);
    vm_add_built_in(vm, MATTE_EXT_CALL_GET_OPERATOR,  1, vm_ext_call__get_operator);

    vm_add_built_in(vm, MATTE_EXT_CALL_INTROSPECT, 0, vm_ext_call__introspect);
    vm_add_built_in(vm, MATTE_EXT_CALL_TYPE,       1, vm_ext_call__newtype);
    vm_add_built_in(vm, MATTE_EXT_CALL_INSTANTIATE,1, vm_ext_call__instantiate);
    vm_add_built_in(vm, MATTE_EXT_CALL_PRINT,      1, vm_ext_call__print);
    vm_add_built_in(vm, MATTE_EXT_CALL_SEND,       1, vm_ext_call__send);
    vm_add_built_in(vm, MATTE_EXT_CALL_LISTEN,     2, vm_ext_call__listen);
    vm_add_built_in(vm, MATTE_EXT_CALL_ERROR,      1, vm_ext_call__error);

    vm_add_built_in(vm, MATTE_EXT_CALL_GETEXTERNALFUNCTION, 1, vm_ext_call__getexternalfunction);


    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_TYPE, 1, vm_ext_call__introspect_type);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_KEYS, 1, vm_ext_call__introspect_keys);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_VALUES, 1, vm_ext_call__introspect_values);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_KEYCOUNT, 1, vm_ext_call__introspect_keycount);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_ISCALLABLE, 1, vm_ext_call__introspect_iscallable);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_ARRAYTOSTRING, 1, vm_ext_call__introspect_arraytostring);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_LENGTH, 1, vm_ext_call__introspect_length);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_CHARAT, 2, vm_ext_call__introspect_charat);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_CHARCODEAT, 2, vm_ext_call__introspect_charcodeat);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_SUBSET, 3, vm_ext_call__introspect_subset);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_FLOOR, 1, vm_ext_call__introspect_floor);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_CEIL, 1, vm_ext_call__introspect_ceil);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_ROUND, 1, vm_ext_call__introspect_round);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_TORADIANS, 1, vm_ext_call__introspect_toradians);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_TODEGREES, 1, vm_ext_call__introspect_todegrees);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_SIN, 1, vm_ext_call__introspect_sin);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_COS, 1, vm_ext_call__introspect_cos);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_TAN, 1, vm_ext_call__introspect_tan);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_ABS, 1, vm_ext_call__introspect_abs);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_SQRT, 1, vm_ext_call__introspect_sqrt);    
    vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_ISNAN, 1, vm_ext_call__introspect_isnan);    

    //vm_add_built_in(vm, MATTE_EXT_CALL_INTERNAL__INTROSPECT_NOWRITE, 0, vm_ext_call__introspect_nowrite);    


    return vm;
}

void matte_vm_destroy(matteVM_t * vm) {
    matteTableIter_t * iter = matte_table_iter_create();
    matteTableIter_t * subiter = matte_table_iter_create();

    for(matte_table_iter_start(iter, vm->imported);
        !matte_table_iter_is_end(iter);
        matte_table_iter_proceed(iter)) {
        
        matteValue_t * v = matte_table_iter_get_value(iter);
        matte_value_object_pop_lock(*v);
        matte_heap_recycle(*v);
        free(v);
    }
    matte_table_destroy(vm->imported);
    introspection_destroy_table(vm);
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




    uint32_t i;
    uint32_t len = matte_array_get_size(vm->callstack);
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
    const matteString_t * prettyName
) {
    if (vm->pendingCatchable) return matte_heap_new_value(vm->heap);
    int callable = matte_value_is_callable(func); 
    if (!callable) {
        matteString_t * err = matte_string_create_from_c_str("Error: cannot call non-function value ");
        if (prettyName)
            matte_string_concat(err, prettyName);
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_heap_new_value(vm->heap);
    }

    // special case: type object as a function
    if (func.binID == MATTE_VALUE_TYPE_TYPE) {                
        if (matte_array_get_size(args)) {
            return matte_value_to_type(matte_array_at(args, matteValue_t, 0), func);
        } else {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("Type conversion failed (no value given to convert)."));
            return matte_heap_new_value(vm->heap);
        }
    }


    // normal function
    matteValue_t d = matte_heap_new_value(vm->heap);
    matte_value_into_copy(&d, func);

    if (matte_bytecode_stub_get_file_id(matte_value_get_bytecode_stub(d)) == 0) {
        // fileid 0 is a special fileid that never refers to a real file.
        // this is used to call external c functions. stubID refers to 
        // which index within externalFunction.
        // In this case, a new stackframe is NOT pushed.
        uint32_t external = matte_bytecode_stub_get_id(matte_value_get_bytecode_stub(d));
        if (external >= matte_array_get_size(vm->externalFunctionIndex)) {
            matte_heap_recycle(d);
            return matte_heap_new_value(vm->heap);            
        }
        ExternalFunctionSet_t * set = &matte_array_at(vm->externalFunctionIndex, ExternalFunctionSet_t, external);

        uint32_t i;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(matte_value_get_bytecode_stub(d));
        matteArray_t * argsReal = matte_array_create(sizeof(matteValue_t));
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);

            // copy as many args as there are available.
            // All remaining args are empty.
            if (i < lenReal) {
                matte_value_into_copy(&v, matte_array_at(args, matteValue_t, i));

                // sicne this function doesn't use a referrable, we need to set roots manually.
                if (v.binID == MATTE_VALUE_TYPE_OBJECT) {
                    matte_value_object_push_lock(v);
                }
            }

            matte_array_push(argsReal, v);
        }
        matte_value_object_push_lock(d);
        matteValue_t result = {};
        if (callable == 2) {
            int ok = matte_value_object_function_pre_typecheck_unsafe(d, argsReal);
            if (ok) 
                result = set->userFunction(vm, d, argsReal, set->userData);
            matte_value_object_function_post_typecheck_unsafe(d, result);
        } else {
            result = set->userFunction(vm, d, argsReal, set->userData);        
        }
        matte_value_object_pop_lock(d);


        len = matte_array_get_size(argsReal);
        for(i = 0; i < len; ++i) {
            if (matte_array_at(argsReal, matteValue_t, i).binID == MATTE_VALUE_TYPE_OBJECT) {
                matte_value_object_pop_lock(matte_array_at(argsReal, matteValue_t, i));
            }
            matte_heap_recycle(matte_array_at(argsReal, matteValue_t, i));
        }
        matte_array_destroy(argsReal);
        matte_heap_recycle(d);
        return result;
    } else {
        // no calling context yet
        matteVMStackFrame_t * frame = vm_push_frame(vm);
        if (prettyName) {
            matte_string_set(frame->prettyName, prettyName);
        }
        frame->context = d;
        frame->stub = matte_value_get_bytecode_stub(d);

        matteArray_t * referrables = matte_array_create(sizeof(matteValue_t));

        matteValue_t empty = matte_heap_new_value(vm->heap);

        // slot 0 is always the context
        matte_array_push(referrables, frame->context);
        uint32_t i;
        uint32_t lenReal = matte_array_get_size(args);
        uint32_t len = matte_bytecode_stub_arg_count(frame->stub);
        for(i = 0; i < len; ++i) {
            // copy as many args as there are available.
            // All remaining args are empty.

            if (i < lenReal) {
                matte_array_push(referrables, matte_array_at(args, matteValue_t, i));
            } else {
                matte_array_push(referrables, empty);
            }
        }
        int ok = 1;
        if (callable == 2 && len) { // typestrictcheck
            ok = matte_value_object_function_pre_typecheck_unsafe(
                d, 
                MATTE_ARRAY_CAST(
                    ((matteValue_t*)matte_array_get_data(referrables))+1,
                    matteValue_t,
                    len 
                )
            );
        }
        len = matte_bytecode_stub_local_count(frame->stub);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_array_push(referrables, v);
        }
        
        frame->referrable = matte_heap_new_value(vm->heap);
        // ref copies of values happen here.
        matte_value_into_new_object_array_ref(&frame->referrable, referrables);
        #ifdef MATTE_DEBUG__HEAP
        {
            uint32_t instcount;
            const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame->stub, &instcount);                    
            matteString_t * info = matte_string_create_from_c_str("REFERRABLE MADE @");
            if (matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub))) {
                matte_string_concat(info, matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(frame->stub)));
            }
            matte_heap_track_neutral(frame->referrable, matte_string_get_c_str(info), inst->lineNumber);
            matte_string_destroy(info);
        }
        #endif
        
        
        frame->referrableSize = matte_array_get_size(referrables);
        matte_array_destroy(referrables);

        // establishes the reference path of objects not allowed to be cleaned up
        matte_value_object_push_lock(frame->referrable);
        matte_value_object_push_lock(frame->context);        
        matteValue_t result = matte_heap_new_value(vm->heap);
        if (callable == 2) {
            if (ok) 
                result = vm_execution_loop(vm);
            matte_value_object_function_post_typecheck_unsafe(d, result);
        } else {
            result = vm_execution_loop(vm);        
        }
        matte_value_object_pop_lock(frame->referrable);
        matte_value_object_pop_lock(frame->context);

        // cleanup;
        matte_heap_recycle(frame->context);
        matte_heap_recycle(frame->referrable);
        matte_value_object_push_lock(result);
        matte_heap_garbage_collect(vm->heap);
        matte_value_object_pop_lock(result);
        vm_pop_frame(vm);


        

        // uh oh... unhandled errors...
        if (!vm->stacksize && vm->pendingCatchable) {
            if (vm->debug) {
                vm->debug(
                    vm,
                    MATTE_VM_DEBUG_EVENT__UNHANDLED_ERROR_RAISED,
                    0,
                    0,
                    vm->catchable,
                    vm->debugData                   
                );                
            }     
            #ifdef MATTE_DEBUG
                printf("VM Error: UNHANDLED CATCHABLE: %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_value_as_string(vm->catchable))));
            #endif
            matte_value_object_pop_lock(vm->catchable);
            vm->catchable.binID = 0;
            vm->pendingCatchable = 0;
        }
        return result; // ok, vm_execution_loop returns new
    } 
}

matteValue_t matte_vm_run_script(
    matteVM_t * vm, 
    uint32_t fileid, 
    const matteArray_t * args
) {
    matteValue_t func = matte_heap_new_value(vm->heap);
    matteBytecodeStub_t * stub = vm_find_stub(vm, fileid, 0);
    if (!stub) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Script has no toplevel context to run."));
        return func;
    }
    matte_value_into_new_function_ref(&func, stub);
    matte_value_object_push_lock(func);
    matteValue_t result = matte_vm_call(vm, func, args, matte_vm_get_script_name_by_id(vm, fileid));

    matte_value_object_pop_lock(func);
    matte_heap_recycle(func);
    return result;
}


static void debug_compile_error(
    const matteString_t * s,
    uint32_t line,
    uint32_t ch,
    void * data
) {
    matteVM_t * vm = data;
    matteValue_t v = matte_heap_new_value(vm->heap);
    matte_value_into_string(&v, s);
    vm->debug(
        vm,
        MATTE_VM_DEBUG_EVENT__ERROR_RAISED,
        MATTE_VM_DEBUG_FILE,
        line, 
        v,
        vm->debugData
    );
    matte_heap_recycle(v);
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

    vm->catchable.binID = 0;
    vm->pendingCatchable = 0;
 
    uint32_t jitSize = 0;
    uint8_t * jitBuffer = matte_compiler_run_with_named_references(
        matte_string_get_c_str(src), // TODO: UTF8
        matte_string_get_length(src),
        &jitSize,
        debug_compile_error,
        vm
    ); 
    matteValue_t result;
    if (jitSize) {
        matteArray_t * jitstubs = matte_bytecode_stubs_from_bytecode(
            MATTE_VM_DEBUG_FILE, jitBuffer, jitSize
        );
        
        matte_vm_add_stubs(vm, jitstubs);
        result = matte_vm_run_script(vm, MATTE_VM_DEBUG_FILE, matte_array_empty());

    } else {
        result = matte_heap_new_value(vm->heap);
    }
    
    // revert vm state 
    vm->debug = realDebug;
    vm->debugData = realDebugData;
    matte_value_object_pop_lock(vm->catchable);        
    matte_heap_recycle(vm->catchable);      
    
      
    vm->pendingCatchable = pendingCatchable;
    vm->catchable = catchable;
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
    matte_value_into_new_object_ref(&out);
    
    matteValue_t callstack = matte_heap_new_value(vm->heap);
    matte_value_into_new_object_ref(&callstack);
    
    matteValue_t key = matte_heap_new_value(vm->heap);
    matteValue_t val = matte_heap_new_value(vm->heap);

    uint32_t i;
    uint32_t len = matte_vm_get_stackframe_size(vm);
    matte_value_into_string(&key, MATTE_STR_CAST("length"));
    matte_value_into_number(&val, len);
    matte_value_object_set(callstack, key, val, 0);
    
    
    matteValue_t * arr = malloc(sizeof(matteValue_t) * len);
    matteString_t * str = matte_string_create_from_c_str("Matte Language VM callstack (%d entries):\n", len);
    
    for(i = 0; i < len; ++i) {
        matteVMStackFrame_t framesrc = matte_vm_get_stackframe(vm, i);

        matteValue_t frame = matte_heap_new_value(vm->heap);
        matte_value_into_new_object_ref(&frame);

        const matteString_t * filename = matte_vm_get_script_name_by_id(vm, matte_bytecode_stub_get_file_id(framesrc.stub));
        
        matte_value_into_string(&key, MATTE_STR_CAST("file"));
        matte_value_into_string(&val, filename ? filename : MATTE_STR_CAST("<unknown>"));        
        matte_value_object_set(frame, key, val, 0);
        

        
        
        uint32_t instcount = 0;
        const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(framesrc.stub, &instcount);        
        
        matte_value_into_string(&key, MATTE_STR_CAST("lineNumber"));        
        int lineNumber = 0;
        if (framesrc.pc - 1 < instcount) {
            lineNumber = inst[framesrc.pc-1].lineNumber;                
        } 
        matte_value_into_number(&val, lineNumber);
        matte_value_object_set(frame, key, val, 0);
        arr[i] = frame;        
        
        matte_string_concat_printf(str, " (%d) -> %s, line %d\n", i, filename ? matte_string_get_c_str(filename) : "<unknown>", lineNumber); 
    }


    matte_value_into_new_object_array_ref(&val, MATTE_ARRAY_CAST(arr, matteValue_t, len));
    free(arr);
    matte_value_into_string(&key, MATTE_STR_CAST("frames"));
    matte_value_object_set(callstack, key, val, 0);
    

    
    
    matte_value_into_string(&key, MATTE_STR_CAST("callstack"));    
    matte_value_object_set(out, key, callstack, 0);


    matte_value_into_string(&key, MATTE_STR_CAST("summary"));
    matte_value_into_string(&val, str);
    matte_string_destroy(str);
    matte_value_object_set(out, key, val, 0);

    matte_value_into_string(&key, MATTE_STR_CAST("data"));
    matte_value_object_set(out, key, detail, 0);    

    matte_heap_recycle(val);
    matte_heap_recycle(key);
    matte_heap_recycle(callstack);

    return out;
}


void matte_vm_raise_error(matteVM_t * vm, matteValue_t val) {
    if (vm->pendingCatchable) {
        #ifdef MATTE_DEBUG
            assert(!"VM has a new error generated before previous error could be captured. This is not allowed and is /probably/ indicative of internal VM error.");
        #endif
    }

    matteValue_t info = vm_info_new_object(vm, val);
    vm->catchable = info;
    vm->pendingCatchable = 1;
    matte_value_object_push_lock(info);
    
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
    matte_value_into_string(&b, str);
    matte_vm_raise_error(vm, b);
    matte_heap_recycle(b);
}


// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i) {
    matteVMStackFrame_t ** frames = matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested."));
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
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested in referrable query."));
        return NULL;
    }


    // get context
    if (referrableID < frames[i]->referrableSize) {
        return matte_value_object_array_at_unsafe(frames[i]->referrable, referrableID);        
    } else {
        matteValue_t * ref = matte_value_get_captured_value(frames[i]->context, referrableID - frames[i]->referrableSize);

        // bad referrable
        if (!ref) {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid referrable"));
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
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested in referrable assignment."));
        return;
    }


    // get context
    if (referrableID < frames[i]->referrableSize) {
        matteValue_t vkey = matte_heap_new_value(vm->heap);
        matte_value_into_number(&vkey, referrableID);
        matte_value_object_set(frames[i]->referrable, vkey, val, 1);
        matte_heap_recycle(vkey);
        //matte_value_into_copy(matte_value_object_array_at_unsafe(frames[i]->referrable, referrableID), val);
    } else {
        matte_value_set_captured_value(
            frames[i]->context, 
            referrableID - frames[i]->referrableSize,
            val
        );
    }
}


void matte_vm_set_external_function(
    matteVM_t * vm, 
    const matteString_t * identifier,
    uint8_t nArgs,
    matteValue_t (*userFunction)(matteVM_t *, matteValue_t, matteArray_t * args, void * userData),
    void * userData
) {
    if (!userFunction) return;



    ExternalFunctionSet_t * set;    
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
        nArgs, 
        userFunction
    );


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

    matteBytecodeStub_t * stub = matte_array_at(vm->extStubs, matteBytecodeStub_t *, *id);

    matteValue_t func = {};
    func.heap = vm->heap;
    matte_value_into_new_function_ref(&func, stub);
    return func;
}



void matte_vm_set_debug_callback(
    matteVM_t * vm,
    void(*debugCB)(matteVM_t *, matteVMDebugEvent_t event, uint32_t file, int lineNumber, matteValue_t, void *),
    void * data
) {
    vm->debug = debugCB;
    vm->debugData = data;
}
