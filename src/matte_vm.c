#include "matte_vm.h"

struct matteVM_t {
    matteArray_t * stubs;

    // stubIndex[fileid] -> [stubid]
    matteTable_t * stubIndex;

    // topmost stub: current frame
    matteArray_t * callstack;
    
    // number of stackframes in use.
    uint32_t stacksize;    
    
    
    // operations pending that MUST be executed BEFORE the pc continues.
    // consists of matteBytecodeStubInstruction_t
    matteArray_t * interruptOps;
    
    
    // for all values
    matteHeap_t * heap;
      
};


static matteVMStackFrame_t * vm_push_frame(matteVM_t * vm) {
    stackSize++;
    matteVMStackFrame_t * frame;
    if (matte_array_get_size(vm->callstack) < stackSize) {
        matte_array_set_size(vm->callstack, stackSize);
        frame = &matte_array_at(vm->callstack, matteVMStackFrame_t, stackSize-1);

        // initialize
        frame->pc = 0;
        frame->prettyName = matte_string_create();
        frame->context = NULL;
        frame->stub = NULL;
        frame->arguments = matte_array_create(sizeof(matteValue_t));
        frame->locals = matte_array_create(sizeof(matteValue_t));
        frame->captured = matte_array_create(sizeof(matteValue_t));
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



static void vm_execution_loop(matteVM_t * vm) {
    matteVMStackFrame_t * frame = matte_vm_get_stackframe(vm, 0);
    matteBytecodeStubInstruction_t * inst;
    uint32_t instCount;
    matteBytecodeStubInstruction_t * program = matte_bytecode_stub_get_instructions(frame->stub, &instCount);

    while(frame->pc < instCount) {
        inst = program[frame->pc++];
        switch(inst->opcode) {
          case MATTE_OPCODE_NOP:
            break;
            
          case MATTE_OPCODE_NEM:
            a        
        
        }
    }
}


matteVM_t * matte_vm_create() {
    matteVM_t * vm = calloc(1, sizeof(matteVM_t));
    vm->stubs = matte_array_create(sizeof(matteBytecodeStub_t*));
    vm->callstack = matte_array_create(sizeof(matteVMStackFrame_t));
    vm->stubIndex = matte_table_create_hash_pointer();
    vm->interruptOps = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

    return vm;
}




matteValue_t matte_vm_run_stub(
    matteVM_t * vm, 
    uint16_t fileid, 
    const matteArray_t * args
) {
    // no calling context yet
    if (vm->stackSize == 0) {
        matteVMStackFrame_t * frame = vm_push_frame(vm);
        frame->stub = vm_find_stub(fileid, 0);
        if (frame->stub) {
            vm_execution_loop(vm);
        } else {
            #ifdef MATTE_DEBUG
                printf("fileid has no toplevel stub. Nothing to execute.\n");
            #endif
        }
    } else {
        // interrupt the current callstack to call as soon as interrupt is over;
        matteVMStackFrame_t * frame = matte_vm_get_stackframe(vm, 0);
        matteBytecodeStubInstruction_t op;
        op.opcode = MATTE_OPCODE_NFN;
        uint32_t * arr = (uint32_t*)op.data;
        arr[0] = fileid;
        arr[1] = 0;
        op.lineNumber = -1; // not in a file.
        matte_array_push(vm->interruptOps, op);

        // call the new function
        op.opcode = MATTE_OPCODE_CAL;
        matte_array_push(vm->interruptOps, op);
    }
}

