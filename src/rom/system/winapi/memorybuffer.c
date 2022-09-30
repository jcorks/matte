
#define MEMORYBUFFER_ID_TAG 0xf3f01bbf

typedef struct {
    uint8_t * buffer;
    uint64_t size;
    uint64_t alloc;
    uint32_t idval;
} MatteMemoryBuffer;


static void auto_cleanup_buffer(void * ud, void * fd) {
    if (ud) {
        MatteMemoryBuffer * b = ud;
        free(b->buffer);
        free(b);
    }
}

// helper for other system implementations that 
// deal with raw data: 
static matteValue_t matte_system_shared__create_memory_buffer_from_raw(matteVM_t * vm, const uint8_t * data, uint32_t size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &out);
    matte_value_object_set_native_finalizer(heap, out, auto_cleanup_buffer, NULL);    
    MatteMemoryBuffer * m = calloc(1, sizeof(MatteMemoryBuffer));
    m->idval = MEMORYBUFFER_ID_TAG;
    matte_value_object_set_userdata(heap, out, m);    

    
    if (data && size) {    
        m->buffer = malloc(size);
        memcpy(m->buffer, data, size);
        m->alloc = size;
        m->size = size;
    }
    
    return out;
}

const uint8_t * matte_system_shared__get_raw_from_memory_buffer(matteVM_t * vm, matteValue_t b, uint32_t * size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteMemoryBuffer * mB = matte_value_object_get_userdata(heap, b);
    if (!(mB && mB->idval == MEMORYBUFFER_ID_TAG)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Invalid memory buffer instance."));
        *size = 0;
        return NULL;                
    }
    
    *size = mB->size;
    return mB->buffer;
}


MATTE_EXT_FN(matte_ext__memory_buffer__create) {
    return matte_system_shared__create_memory_buffer_from_raw(vm, NULL, 0);
}



MATTE_EXT_FN(matte_ext__memory_buffer__release) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);
    m->idval = 0;
    free(m->buffer);
    free(m);
    matte_value_object_set_userdata(heap, a, NULL);
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint64_t length = matte_value_as_number(heap, args[1]);
    if (m->alloc < length) {
        m->alloc = length;
        m->buffer = realloc(m->buffer, length);
    }
    // no such thing as uninitialized
    memset(m->buffer+m->size, 0, length - m->size);    
    m->size = length;
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__copy) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(heap, a);
    matteValue_t b = args[2];
    MatteMemoryBuffer * mB = matte_value_object_get_userdata(heap, b);

    uint64_t offsetA = matte_value_as_number(heap, args[1]);
    uint64_t offsetB = matte_value_as_number(heap, args[3]);
    uint64_t length  = matte_value_as_number(heap, args[4]);


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size ||
        offsetB > mB->size ||
        offsetB + length > mB->size) {
        
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Copy of buffer failed: The copy is outside the range of either the source or destination."));
        return matte_heap_new_value(heap);           
    }

    memcpy(mA->buffer+offsetA, mB->buffer+offsetB, length);
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(heap, a);

    uint64_t offsetA = matte_value_as_number(heap, args[1]);
    uint8_t  val     = matte_value_as_number(heap, args[2]);
    uint64_t length  = matte_value_as_number(heap, args[3]);


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size) {
        
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Set of buffer failed: The copy is outside the range of destination."));
        return matte_heap_new_value(heap);           
    }

    memset(mA->buffer+offsetA, val, length);
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_ext__memory_buffer__subset) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint64_t from = matte_value_as_number(heap, args[1]);
    uint64_t to   = matte_value_as_number(heap, args[2]);

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Subset call failed: Improper indices for buffer"));
        return matte_heap_new_value(heap);                   
    }


    return matte_system_shared__create_memory_buffer_from_raw(
        vm, 
        m->buffer+from, 
        (to-from)+1
    ); 
}


MATTE_EXT_FN(matte_ext__memory_buffer__append_byte) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint8_t val   = matte_value_as_number(heap, args[1]);

    if (m->alloc == m->size) {
        m->alloc = 10 + m->alloc*1.1;
        m->buffer = realloc(m->buffer, m->alloc);
    }
    m->buffer[m->size++] = val;
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_ext__memory_buffer__append) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(heap, a);
    matteValue_t b = args[1];
    MatteMemoryBuffer * mB = matte_value_object_get_userdata(heap, b);




    if (mA->alloc < mA->size + mB->size) {
        mA->alloc = mA->size + mB->size;
        mA->buffer = realloc(mA->buffer, mA->alloc);
    }
    memcpy(mA->buffer+mA->size, mB->buffer, mB->size);
    mA->size += mB->size;
    
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_ext__memory_buffer__remove) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint64_t from = matte_value_as_number(heap, args[1]);
    uint64_t to   = matte_value_as_number(heap, args[2]);

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Remove call failed: Improper indices for buffer"));
        return matte_heap_new_value(heap);                   
    }


    memmove(m->buffer+from, m->buffer+to+1, m->size - (to+1));
    m->size-=(to-from)+1;
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_ext__memory_buffer__get_size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, m->size);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__get_index) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint64_t from = matte_value_as_number(heap, args[1]);
    if (from >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not get value from buffer: index out of range."));
        return matte_heap_new_value(heap);                   

    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, m->buffer[from]);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__as_utf8) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    char * buf = malloc(m->size+1);
    buf[m->size] = 0;
    memcpy(buf, m->buffer, m->size);
    
    matteString_t * outStr = matte_string_create_from_c_str("%s", buf);
    free(buf);
    
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, outStr);
    matte_string_destroy(outStr);
    return out;
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_index) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);

    uint64_t from = matte_value_as_number(heap, args[1]);
    uint8_t  val  = matte_value_as_number(heap, args[2]);
    if (from >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not set value in buffer: index out of range."));
        return matte_heap_new_value(heap);                   
    }

    m->buffer[from] = val;
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, m->buffer[from]);
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__read_primitive) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);
    uint64_t offset = matte_value_as_number(heap, args[1]);


    int byteSize = 1;
    int primitive = matte_value_as_number(heap, args[2]);

    switch(primitive) {
      case 4:
      case 0: byteSize = 1; break;
      
      case 1:
      case 5: byteSize = 2; break;
      
      case 2:
      case 6:
      case 8: byteSize = 4; break;
      
      case 3:
      case 7:
      case 9: byteSize = 8; break;
    };

    if (offset+byteSize-1 >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not read value: index out of range."));
        return matte_heap_new_value(heap);                   
    }




    double val = 0; // best we can do. for now.
    switch(primitive) {
      case 0: val = (double)*((int8_t*) (m->buffer+offset)); break;
      case 1: val = (double)*((int16_t*)(m->buffer+offset)); break;
      case 2: val = (double)*((int32_t*)(m->buffer+offset)); break;
      case 3: val = (double)*((int64_t*)(m->buffer+offset)); break;
      case 4: val = (double)*((uint8_t*) (m->buffer+offset)); break;
      case 5: val = (double)*((uint16_t*) (m->buffer+offset)); break;
      case 6: val = (double)*((uint32_t*) (m->buffer+offset)); break;
      case 7: val = (double)*((uint64_t*) (m->buffer+offset)); break;
      case 8: val = (double)*((float*) (m->buffer+offset)); break;
      case 9: val = (double)*((double*) (m->buffer+offset)); break;


    }


    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, val);
    return out;    
}



MATTE_EXT_FN(matte_ext__memory_buffer__write_primitive) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = matte_value_object_get_userdata(heap, a);
    uint64_t offset = matte_value_as_number(heap, args[1]);
    

    int byteSize = 1;
    int primitive = matte_value_as_number(heap, args[2]);

    switch(primitive) {
      case 4:
      case 0: byteSize = 1; break;
      
      case 1:
      case 5: byteSize = 2; break;
      
      case 2:
      case 6:
      case 8: byteSize = 4; break;
      
      case 3:
      case 7:
      case 9: byteSize = 8; break;
    };

    if (offset+byteSize-1 >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not read value: index out of range."));
        return matte_heap_new_value(heap);                   
    }




    double val = matte_value_as_number(heap, args[3]); // best we can do. for now.
    switch(primitive) {
      case 0: *((int8_t*) (m->buffer+offset)) = val; break;
      case 1: *((int16_t*)(m->buffer+offset)) = val; break;
      case 2: *((int32_t*)(m->buffer+offset)) = val; break;
      case 3: *((int64_t*)(m->buffer+offset)) = val; break;
      case 4: *((uint8_t*) (m->buffer+offset)) = val; break;
      case 5: *((uint16_t*) (m->buffer+offset)) = val; break;
      case 6: *((uint32_t*) (m->buffer+offset)) = val; break;
      case 7: *((uint64_t*) (m->buffer+offset)) = val; break;
      case 8: *((float*) (m->buffer+offset)) = val; break;
      case 9: *((double*) (m->buffer+offset)) = val; break;
    }


    return matte_heap_new_value(heap);
}








static void matte_system__memorybuffer(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_create"),        0, matte_ext__memory_buffer__create,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_release"),       1, matte_ext__memory_buffer__release,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set_size"),      2, matte_ext__memory_buffer__set_size,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_copy"),          5, matte_ext__memory_buffer__copy,       NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set"),           4, matte_ext__memory_buffer__set,        NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_subset"),        3, matte_ext__memory_buffer__subset,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_append_byte"),   2, matte_ext__memory_buffer__append_byte,NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_append"),        2, matte_ext__memory_buffer__append     ,NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_remove"),        3, matte_ext__memory_buffer__remove,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_get_size"),      1, matte_ext__memory_buffer__get_size,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_get_index"),     2, matte_ext__memory_buffer__get_index,  NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set_index"),     3, matte_ext__memory_buffer__set_index,  NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_as_utf8"),       1, matte_ext__memory_buffer__as_utf8,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_write_primitive"),4, matte_ext__memory_buffer__write_primitive,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_read_primitive"), 3, matte_ext__memory_buffer__read_primitive,    NULL);

 
}
