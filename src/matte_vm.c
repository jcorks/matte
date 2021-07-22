#include "matte_vm.h"
#include "matte_table.h"
#include "matte_array.h"
#include "matte_heap.h"
#include "matte_bytecode_stub.h"
#include "matte_string.h"
#include "matte_opcode.h"
#include <stdlib.h>
#include <string.h>
#ifdef MATTE_DEBUG
    #include <stdio.h>
    #include <assert.h>
#endif

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
    matteValue_t (*userFunction)(matteVM_t *, matteArray_t * args, void * userData);
    void * userData;
    uint8_t nArgs;
} ExternalFunctionSet_t;

static matteVMStackFrame_t * vm_push_frame(matteVM_t * vm) {
    vm->stacksize++;
    matteVMStackFrame_t * frame;
    if (matte_array_get_size(vm->callstack) < vm->stacksize) {
        matte_array_set_size(vm->callstack, vm->stacksize);
        frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, vm->stacksize-1);

        // initialize
        frame->pc = 0;
        frame->prettyName = matte_string_create();
        frame->context = matte_heap_new_value(vm->heap); // will contain captures
        frame->stub = NULL;
        frame->arguments = matte_array_create(sizeof(matteValue_t));
        frame->locals = matte_array_create(sizeof(matteValue_t));
        frame->valueStack = matte_array_create(sizeof(matteValue_t));
    } else {
        frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, vm->stacksize-1);
        frame->stub = NULL;
        matte_string_clear(frame->prettyName);     
        matte_array_set_size(frame->arguments, 0);
        matte_array_set_size(frame->locals, 0);
        matte_array_set_size(frame->arguments, 0);
           
    }
    return frame;
}


static void vm_pop_frame(matteVM_t * vm) {
    if (vm->stacksize == 0) return; // error??    
    vm->stacksize--;
}


static matteBytecodeStub_t * vm_find_stub(matteVM_t * vm, uint16_t fileid, uint16_t stubid) {
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
      case MATTE_OPERATOR_POS:        return vm_operator__pow         (vm, a, b);

      case MATTE_OPERATOR_LESS: return vm_operator__less(vm, a, b);

      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled OPR operator"));                        
        return matte_heap_new_value(vm->heap);
    }
}


static matteValue_t vm_operator_1(matteVM_t * vm, matteOperator_t op, matteValue_t a) {
    switch(op) {
      case MATTE_OPERATOR_TONUMBER: return vm_operator__tonumber(vm, a);
      case MATTE_OPERATOR_NOT:      return vm_operator__not(vm, a);

      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled OPR operator"));                        
        return matte_heap_new_value(vm->heap);
    }
}


static matteValue_t vm_ext_call_3(matteVM_t * vm, uint64_t call, matteValue_t a, matteValue_t b, matteValue_t c) {
    switch(call) {
      case MATTE_EXT_CALL_GATE: return vm_ext_call__gate(vm, a, b, c);
      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled EXT operation"));                        
        return matte_heap_new_value(vm->heap);      
    }

}

static matteValue_t vm_ext_call_2(matteVM_t * vm, uint64_t call, matteValue_t a, matteValue_t b) {
    switch(call) {
      case MATTE_EXT_CALL_WHILE: return vm_ext_call__while(vm, a, b);
      default:
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("unhandled EXT operation"));                        
        return matte_heap_new_value(vm->heap);
      
    }
}


static const char * opcode_to_str(int oc) {
    switch(oc) {
      case MATTE_OPCODE_NOP: return "NOP";
      case MATTE_OPCODE_PRF: return "PRF";
      case MATTE_OPCODE_NEM: return "NEM";
      case MATTE_OPCODE_NNM: return "NNM";
      case MATTE_OPCODE_NST: return "NST";
      case MATTE_OPCODE_STC: return "STC";
      case MATTE_OPCODE_NOB: return "NOB";
      case MATTE_OPCODE_NFN: return "NFN";
        
        
      case MATTE_OPCODE_CAL: return "CAL";
      case MATTE_OPCODE_ARF: return "ARF";
      case MATTE_OPCODE_OSN: return "OSN";
      case MATTE_OPCODE_OPR: return "OPR";
      case MATTE_OPCODE_EXT: return "EXT";
      case MATTE_OPCODE_POP: return "POP";
      case MATTE_OPCODE_RET: return "RET";
      case MATTE_OPCODE_SKP: return "SKP";
      default:
        return "???";
    }
}

#define STACK_SIZE() matte_array_get_size(frame->valueStack)
#define STACK_POP() matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1); matte_array_set_size(frame->valueStack, matte_array_get_size(frame->valueStack)-1);
#define STACK_PUSH(__v__) matte_array_push(frame->valueStack, __v__);

static matteValue_t vm_execution_loop(matteVM_t * vm) {
    matteVMStackFrame_t * frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, matte_array_get_size(vm->callstack)-1);
    const matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    const matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);
    matteValue_t output = matte_heap_new_value(vm->heap);



    while(frame->pc < instCount) {
        inst = program+frame->pc++;
        #ifdef MATTE_DEBUG
            printf("CALLSTACK%6d PC%6d, OPCODE %s, Stacklen: %10d\n", vm->stacksize, frame->pc, opcode_to_str(inst->opcode), matte_array_get_size(frame->valueStack));
        #endif

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

          case MATTE_OPCODE_NST: {
            matteString_t * str = matte_string_create();

            uint32_t vals[2];

            if (frame->pc < instCount)
                inst = program+(frame->pc++);

            while(inst->opcode == MATTE_OPCODE_STC) {
                memcpy(vals, inst->data, 8);
                if (vals[0] != 0) matte_string_append_char(str, vals[0]);
                if (vals[1] != 0) matte_string_append_char(str, vals[1]);

                if (frame->pc < instCount)
                    inst = program+(frame->pc++);
            }

            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_string(&v, str);
            matte_string_destroy(str);
            STACK_PUSH(v);
            frame->pc--; // put back prev command
            break;
          }
          case MATTE_OPCODE_NOB: {
            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_new_object_ref(&v);
            STACK_PUSH(v);
            break;
          }
          case MATTE_OPCODE_NFN: {
            uint32_t ids[2];
            memcpy(ids, inst->data, 8);
            if (ids[0] == 0) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("NFN opcode is NOT allowed to reference fileid 0?? (corrupt bytecode?)"));
                break;
            }
            matteBytecodeStub_t * stub = vm_find_stub(vm, ids[0], ids[1]);

            if (stub == NULL) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("NFN opcode data referenced non-existent stub (either parser error OR bytecode was reused erroneously)"));
                break;
            }

            matteValue_t v = matte_heap_new_value(vm->heap);
            matte_value_into_new_function_ref(&v, stub);
            STACK_PUSH(v);
            break;
          }

          case MATTE_OPCODE_CAL: {
            if (STACK_SIZE() == 0) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to run CAL opcode when stack is empty."))    ;
                break;
            }

            uint32_t argcount;
            memcpy(&argcount, inst->data, sizeof(uint32_t));
            if (STACK_SIZE() < argcount) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: tried to prepare arguments for a call, but insufficient arguments on the stack."));    
                break;
            }

            matteValue_t function = STACK_POP();


            matteArray_t * args = matte_array_create(sizeof(matteValue_t));
            uint32_t i;
            for(i = 0; i < argcount; ++i) {
                matteValue_t v = STACK_POP();
                matteValue_t c = matte_heap_new_value(vm->heap);
                matte_value_into_copy(&c, v);
                matte_array_push(args, c);
            }

            matteValue_t result = matte_vm_call(vm, function, args);

            for(i = 0; i < argcount; ++i) {
                matteValue_t v = matte_array_at(args, matteValue_t, i);
                matte_heap_recycle(v);
            }
            matte_array_destroy(args);

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
                matteValue_t v = STACK_POP();
                matte_value_into_copy(ref, v);
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
          case MATTE_OPCODE_OSN: {
            if (STACK_SIZE() < 3) {
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: OSN opcode requires 3 on the stack."));                
                break;        
            }
            matteValue_t object = STACK_POP();
            matteValue_t key = STACK_POP();
            matteValue_t val = STACK_POP();
            
            matte_value_object_set(object, key, val);
            
            matte_heap_recycle(key);
            matte_heap_recycle(val);            
            STACK_PUSH(object); // dont recycle since back in stack
            break;
          }    

          case MATTE_OPCODE_EXT: {
            uint64_t call = *(uint64_t*)inst->data;
            switch(call) {
              case MATTE_EXT_CALL_NOOP: break;
              
              case MATTE_EXT_CALL_WHILE: {
                if (STACK_SIZE() < 2) {
                    matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: operation requires 2 arguments."));                
                } else {
                    matteValue_t a = STACK_POP();
                    matteValue_t b = STACK_POP();
                    matteValue_t v = vm_ext_call_2(vm, call, a, b);
                    matte_heap_recycle(a);
                    matte_heap_recycle(b);
                    STACK_PUSH(v); // already refd
                }
                break;
              }
              case MATTE_EXT_CALL_GATE: {
                if (STACK_SIZE() < 2) {
                    matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: operation requires 3 arguments."));                
                } else {
                    matteValue_t a = STACK_POP();
                    matteValue_t b = STACK_POP();
                    matteValue_t c = STACK_POP();
                    matteValue_t v = vm_ext_call_3(vm, call, a, b, c);
                    matte_heap_recycle(a);
                    matte_heap_recycle(b);
                    matte_heap_recycle(c);
                    STACK_PUSH(v); // already refd
                }
                break;
              }
              
              default:;
                matte_vm_raise_error_string(vm, MATTE_STR_CAST("VM error: unknown EXT command"));                
                
            }
            break;
          }
          case MATTE_OPCODE_RET: {
            // ez pz
            break;
          }    
          // used to implement when
          case MATTE_OPCODE_SKP: {
            uint32_t count = *(uint32_t*)inst->data;
            matteValue_t condition = STACK_POP();
            if (!matte_value_as_boolean(condition)) {
                frame->pc += count;
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
                case MATTE_OPERATOR_SHIFT_DOWN:
                case MATTE_OPERATOR_SHIFT_UP:
                case MATTE_OPERATOR_POW:
                case MATTE_OPERATOR_EQ:
                case MATTE_OPERATOR_CDEREF:
                case MATTE_OPERATOR_TERNARY:
                case MATTE_OPERATOR_GREATER:
                case MATTE_OPERATOR_LESS:
                case MATTE_OPERATOR_GREATEREQ:
                case MATTE_OPERATOR_LESSEQ:
                case MATTE_OPERATOR_SPECIFY:
                case MATTE_OPERATOR_TRANSFORM:
                case MATTE_OPERATOR_NOTEQUAL: {
                    if (STACK_SIZE() < 2) {
                        matte_vm_raise_error_string(vm, MATTE_STR_CAST("OPR operator requires 2 operands."));                        
                    } else {
                        matteValue_t a = STACK_POP();
                        matteValue_t b = STACK_POP();
                        matteValue_t v = vm_operator_2(
                            vm,
                            inst->data[0],
                            a, b
                        );
                        matte_heap_recycle(a);
                        matte_heap_recycle(b);
                        STACK_PUSH(v); // ok
                    }
                    break;                
                }
                    
                
                case MATTE_OPERATOR_NOT:
                case MATTE_OPERATOR_BITWISE_NOT:
                case MATTE_OPERATOR_TOSTRING:
                case MATTE_OPERATOR_TONUMBER:
                case MATTE_OPERATOR_TOBOOLEAN:
                case MATTE_OPERATOR_POUND:
                case MATTE_OPERATOR_CURRENCY:
                case MATTE_OPERATOR_TYPENAME: {
                    if (STACK_SIZE() < 1) {
                        matte_vm_raise_error_string(vm, MATTE_STR_CAST("OPR operator requires 1 operand."));                        
                    } else {
                        matteValue_t a = STACK_POP();
                        matteValue_t v = vm_operator_1(
                            vm,
                            inst->data[0],
                            a
                        );
                        matte_heap_recycle(a);
                        STACK_PUSH(v);
                    }
                    break;                
                }
            }
            break;
          }

          case MATTE_OPCODE_STC:
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("STC opcode only allowed as part of NST opcode stream."));    
            break;            
          default: 
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("Unknown / unhandled opcode.")); 
        
        }


        // errors are checked after every instruction.
        // once encountered they are handled immediately.
        // If there is no handler, the error propogates down the callstack.
        if (matte_array_get_size(vm->errors)) {
            matteValue_t v = matte_value_object_access_string(frame->context, MATTE_STR_CAST("errorHandler"));

            // output wont be sent since there was an error. empty will return instead.
            matte_heap_recycle(output);

            if (matte_value_is_callable(v)) {
                matteArray_t * cachedErrors = matte_array_clone(vm->errors);
                matte_array_set_size(vm->errors, 0);

                uint32_t i;
                uint32_t len = matte_array_get_size(vm->errors);
                for(i = 0; i < len; ++i) {
                    matteValue_t err = matte_array_at(vm->errors, matteValue_t, i);
                    matte_vm_call(vm, v, MATTE_ARRAY_CAST(&err, matteValue_t, 1));
                    matte_heap_recycle(err);
                } 
                matte_array_destroy(cachedErrors);
            }

            return matte_heap_new_value(vm->heap);
        }
        
    }



    // top of stack is output
    if (matte_array_get_size(frame->valueStack)) {
        
        matteValue_t output = matte_array_at(frame->valueStack, matteValue_t, matte_array_get_size(frame->valueStack)-1);
        uint32_t i;
        uint32_t len = matte_array_get_size(frame->valueStack);
        for(i = 0; i < len-1; ++i) { 
            matte_heap_recycle(matte_array_at(frame->valueStack, matteValue_t, i));
        }
        matte_array_set_size(frame->valueStack, 0);
        return output; // ok since not removed from normal value stack stuff
    } else {
        return matte_heap_new_value(vm->heap);
    }
}


matteVM_t * matte_vm_create() {
    matteVM_t * vm = calloc(1, sizeof(matteVM_t));
    vm->stubs = matte_array_create(sizeof(matteBytecodeStub_t*));
    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    vm->externalFunctions = matte_table_create_hash_matte_string();
    vm->externalFunctionIndex = matte_array_create(sizeof(ExternalFunctionSet_t));
    vm->heap = matte_heap_create(vm);
    vm->errors = matte_array_create(sizeof(matteValue_t));
    return vm;
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
    const matteArray_t * args
) {
    if (!matte_value_is_callable(func)) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Error: cannot call non-function value."));
        return matte_heap_new_value(vm->heap);
    }

    matteValue_t d = matte_heap_new_value(vm->heap);
    matte_value_into_copy(&d, func);

    if (matte_bytecode_stub_get_file_id(matte_value_get_bytecode_stub(d)) == 0) {
        // fileid 0 is a special fileid that never refers to a real file.
        // this is used to call external c functions. stubID refers to 
        // which index within externalFunction.
        // In this case, a new stackframe is NOT pushed.
        uint16_t external = matte_bytecode_stub_get_id(matte_value_get_bytecode_stub(d));
        if (external >= matte_array_get_size(vm->externalFunctionIndex)) {
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
        matte_heap_recycle(d);
        return result;
    } else {
        // no calling context yet
        matteVMStackFrame_t * frame = vm_push_frame(vm);
        frame->context = d;
        frame->stub = matte_value_get_bytecode_stub(d);

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

        len = matte_bytecode_stub_local_count(frame->stub);
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
        vm_pop_frame(vm);


        

        // uh oh... unhandled errors...
        if (!vm->stacksize && matte_array_get_size(vm->errors)) {
            #ifdef MATTE_DEBUG
                assert(!"Unhandled error generated!!");
            #endif
        }
        matte_heap_recycle(d);
        return result; // ok, vm_execution_loop returns new
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
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Script has no toplevel context to run."));
        return func;
    }
    matte_value_into_new_function_ref(&func, stub);
    matteValue_t result = matte_vm_call(vm, func, args);

    matte_heap_recycle(func);
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
    #ifdef MATTE_DEBUG
        printf("ERROR raised from VM: %s\n", matte_string_get_c_str(str)); 
    #endif
    matte_array_push(vm->errors, b);
}


// Gets the requested stackframe. 0 is the currently running stackframe.
matteVMStackFrame_t matte_vm_get_stackframe(matteVM_t * vm, uint32_t i) {
    matteVMStackFrame_t * frames = matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested."));
        matteVMStackFrame_t err = {0};
        return err;
    }
    return frames[i];
}

matteValue_t * matte_vm_stackframe_get_referrable(matteVM_t * vm, uint32_t i, uint32_t referrableID) {
    matteVMStackFrame_t * frames = matte_array_get_data(vm->callstack);
    i = vm->stacksize - 1 - i;
    if (i >= vm->stacksize) { // invalid or overflowed
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid stackframe requested in referrable query."));
        return NULL;
    }


    // get context
    if (referrableID == 0) {
        return &frames[i].context;
    }

    // next, arguments
    uint32_t offset = 1;
    uint32_t max  = offset+matte_array_get_size(frames[i].arguments);
    if (referrableID < max) {
        return &matte_array_at(frames[i].arguments, matteValue_t, referrableID - offset);        
    }

    // next, locals
    offset = max;
    max  = offset+matte_array_get_size(frames[i].locals);
    if (referrableID < max) {
        return &matte_array_at(frames[i].locals, matteValue_t, referrableID - offset);        
    }

    // finally
    const matteArray_t * captured = matte_value_get_captured_values(frames[i].context);
    offset = max;
    max  = offset+matte_array_get_size(captured);
    if (referrableID < max) {
        return &matte_array_at(captured, matteValue_t, referrableID - offset);        
    }

    // bad referrable
    matte_vm_raise_error_string(vm, MATTE_STR_CAST("Invalid referrable"));
    return NULL;    
}



void matte_vm_set_external_function(
    matteVM_t * vm, 
    matteString_t * identifier,
    uint8_t nArgs,
    matteValue_t (*userFunction)(matteVM_t *, matteArray_t * args, void * userData),
    void * userData
) {
    if (!userFunction) return;



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

    matteArray_t * arr = matte_bytecode_stubs_from_bytecode((uint8_t*)&f, sizeof(uint32_t));
    if (matte_array_get_size(arr) == 0) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("External function conversion failed (truncated stub was denied?)"));
    }
    matteBytecodeStub_t * stub = matte_array_at(arr, matteBytecodeStub_t *, 0);

    matteValue_t func;
    matte_value_into_new_function_ref(&func, stub);
    matte_bytecode_stub_destroy(stub);
    matte_array_destroy(arr);

    return func;
}



