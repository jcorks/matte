#include "../ext.h"

#include <stdlib.h>
#include <string.h>


typedef struct {
    uint8_t * buffer;
    uint64_t size;
    uint64_t alloc;
} MatteMemoryBuffer;


MATTE_EXT_FN(matte_ext__memory_buffer__create) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&out);
    
    
    MatteMemoryBuffer * m = calloc(1, sizeof(MatteMemoryBuffer));
    matte_value_object_set_userdata(out, m);    
    return out;
}

MATTE_EXT_FN(matte_ext__memory_buffer__release) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    free(m->buffer);
    free(m);
    matte_value_object_set_userdata(a, NULL);
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint64_t length = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
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
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(a);
    matteValue_t b = matte_array_at(args, matteValue_t, 2);
    MatteMemoryBuffer * mB = matte_value_object_get_userdata(b);

    uint64_t offsetA = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint64_t offsetB = matte_value_as_number(matte_array_at(args, matteValue_t, 3));
    uint64_t length  = matte_value_as_number(matte_array_at(args, matteValue_t, 4));


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size ||
        offsetB > mB->size ||
        offsetB + length > mB->size) {
        
        matte_vm_raise_error_string(vm, "Copy of buffer failed: The copy is outside the range of either the source or destination.");
        return matte_heap_new_value(heap);           
    }

    memcpy(mA->buffer+offsetA, mB->buffer+offsetB, length);
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(a);

    uint64_t offsetA = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint8_t  val     = matte_value_as_number(matte_array_at(args, matteValue_t, 2));
    uint64_t length  = matte_value_as_number(matte_array_at(args, matteValue_t, 3));


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size)
        
        matte_vm_raise_error_string(vm, "Set of buffer failed: The copy is outside the range of destination.");
        return matte_heap_new_value(heap);           
    }

    memset(mA->buffer+offsetA, val, length);
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_ext__memory_buffer__subset) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint64_t from = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint64_t to   = matte_value_as_number(matte_array_at(args, matteValue_t, 2));

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, "Subset call failed: Improper indices for buffer");
        return matte_heap_new_value(heap);                   
    }


    matteValue_t out = matte_heap_new_value(heap);
    MatteMemoryBuffer * mOut = calloc(1, sizeof(MatteMemoryBuffer));
    matte_value_object_set_userdata(out, m);    
    
    mOut->size = (from-to)+1;
    mOut->alloc = mOut->size;
    mOut->buffer = malloc(mOut->size);
    memcpy(mOut->buffer, m->buffer+from, mOut->size);
    return out;    
}


MATTE_EXT_FN(matte_ext__memory_buffer__append_byte) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint8_t val   = matte_value_as_number(matte_array_at(args, matteValue_t, 1));

    if (m->alloc == m->size) {
        m->alloc = 10 + m->alloc*1.1;
        m->buffer = realloc(m->buffer, m->alloc);
    }
    m->buffer[m->size++] = val;
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_ext__memory_buffer__append) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * mA = matte_value_object_get_userdata(a);
    matteValue_t b = matte_array_at(args, matteValue_t, 1);
    MatteMemoryBuffer * mB = matte_value_object_get_userdata(b);




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
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint64_t from = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint64_t to   = matte_value_as_number(matte_array_at(args, matteValue_t, 2));

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, "Remove call failed: Improper indices for buffer");
        return matte_heap_new_value(heap);                   
    }


    memmove(m->buffer+from, m->buffer+to+1, m->size - (to+1));
    m->size-=(to-from)+1;
    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_ext__memory_buffer__get_size) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(&out, m->size);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__get_index) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint64_t from = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    if (from < m->size) {
        matte_vm_raise_error_string(vm, "Could not get value from buffer: index out of range.");
        return matte_heap_new_value(heap);                   

    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(&out, m->buffer[from]);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_index) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_array_at(args, matteValue_t, 0);
    MatteMemoryBuffer * m = matte_value_object_get_userdata(a);

    uint64_t from = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint8_t  val  = matte_value_as_number(matte_array_at(args, matteValue_t, 2));
    if (from < m->size) {
        matte_vm_raise_error_string(vm, "Could not set value in buffer: index out of range.");
        return matte_heap_new_value(heap);                   

    }

    m->buffer[from] = val;
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(&out, m->buffer[from]);
    return matte_heap_new_value(heap);
}











void matte_ext__memory_buffer(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_create"),        0, matte_ext__memory_buffer__create,     NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_release"),       1, matte_ext__memory_buffer__release,    NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_set_size"),      2, matte_ext__memory_buffer__set_size,   NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_copy"),          5, matte_ext__memory_buffer__copy,       NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_set"),           4, matte_ext__memory_buffer__set,        NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_subset"),        3, matte_ext__memory_buffer__subset,     NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_append_byte"),   2, matte_ext__memory_buffer__append_byte,NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_append"),        2, matte_ext__memory_buffer__append     ,NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_remove"),        3, matte_ext__memory_buffer__remove,     NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_get_size"),      1, matte_ext__memory_buffer__get_size,   NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_get_index"),     2, matte_ext__memory_buffer__get_index,  NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_set_index"),     3, matte_ext__memory_buffer__set_index,  NULL);


 
}