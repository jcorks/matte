#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../native.h"
#include "../../matte.h"
#include "../../matte_string.h"
#include "../../matte_store.h"
#include "../../matte_vm.h"


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
#define MEMORYBUFFER_ID_TAG 0xf3f01bbf

typedef struct {
    uint8_t * buffer;
    uint64_t size;
    uint64_t alloc;
    uint32_t idval;
} MatteMemoryBuffer;


static void auto_cleanup_buffer(void * ud, void * fd) {
    if (ud) {
        MatteMemoryBuffer * b = (MatteMemoryBuffer*)ud;
        matte_deallocate(b->buffer);
        matte_deallocate(b);
    }
}

// helper for other system implementations that 
// deal with raw data: 
matteValue_t matte_system_shared__create_memory_buffer_from_raw(matteVM_t * vm, const uint8_t * data, uint32_t size) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_new_object_ref(store, &out);
    matte_value_object_set_native_finalizer(store, out, auto_cleanup_buffer, NULL);    
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_allocate(sizeof(MatteMemoryBuffer));
    m->idval = MEMORYBUFFER_ID_TAG;
    matte_value_object_set_userdata(store, out, m);    

    
    if (data && size) {    
        m->buffer = (uint8_t*)matte_allocate(size);
        memcpy(m->buffer, data, size);
        m->alloc = size;
        m->size = size;
    }
    
    return out;
}

uint8_t * matte_system_shared__get_raw_from_memory_buffer(matteVM_t * vm, matteValue_t b, uint32_t * size) {
    matteStore_t * store = matte_vm_get_store(vm);
    MatteMemoryBuffer * mB = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, b);
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
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);
    m->idval = 0;
    matte_deallocate(m->buffer);
    matte_deallocate(m);
    matte_value_object_set_userdata(store, a, NULL);
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_size) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t length = matte_value_as_number(store, args[1]);
    if (m->alloc < length) {
        uint64_t oldLength = m->alloc;
        m->alloc = length;
        uint8_t * newBuffer = (uint8_t*)matte_allocate(length);
        memcpy(newBuffer, m->buffer, oldLength);
        matte_deallocate(m->buffer);
        m->buffer = newBuffer;
    }
    // no such thing as uninitialized
    memset(m->buffer+m->size, 0, length - m->size);    
    m->size = length;
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_ext__memory_buffer__copy) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);
    matteValue_t b = args[2];
    MatteMemoryBuffer * mB = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, b);

    uint64_t offsetA = matte_value_as_number(store, args[1]);
    uint64_t offsetB = matte_value_as_number(store, args[3]);
    uint64_t length  = matte_value_as_number(store, args[4]);


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size ||
        offsetB > mB->size ||
        offsetB + length > mB->size) {
        
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Copy of buffer failed: The copy is outside the range of either the source or destination."));
        return matte_store_new_value(store);           
    }

    memcpy(mA->buffer+offsetA, mB->buffer+offsetB, length);
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_ext__memory_buffer__set) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t offsetA = matte_value_as_number(store, args[1]);
    uint8_t  val     = matte_value_as_number(store, args[2]);
    uint64_t length  = matte_value_as_number(store, args[3]);


    // length check
    if (offsetA > mA->size ||
        offsetA + length > mA->size) {
        
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Set of buffer failed: The copy is outside the range of destination."));
        return matte_store_new_value(store);           
    }

    memset(mA->buffer+offsetA, val, length);
    return matte_store_new_value(store);
}



MATTE_EXT_FN(matte_ext__memory_buffer__subset) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t from = matte_value_as_number(store, args[1]);
    uint64_t to   = matte_value_as_number(store, args[2]);

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Subset call failed: Improper indices for buffer"));
        return matte_store_new_value(store);                   
    }


    return matte_system_shared__create_memory_buffer_from_raw(
        vm, 
        m->buffer+from, 
        (to-from)+1
    ); 
}


MATTE_EXT_FN(matte_ext__memory_buffer__append_byte) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint8_t val   = matte_value_as_number(store, args[1]);

    if (m->alloc == m->size) {
        uint64_t prevAlloc = m->alloc;
        m->alloc = 10 + m->alloc*1.1;
        uint8_t * newBuffer = (uint8_t*)matte_allocate(m->alloc);
        memcpy(newBuffer, m->buffer, prevAlloc);
        matte_deallocate(m->buffer);
        m->buffer = newBuffer;
    }
    m->buffer[m->size++] = val;
    return matte_store_new_value(store);
}



MATTE_EXT_FN(matte_ext__memory_buffer__append_utf8) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    const matteString_t * val   = matte_value_string_get_string_unsafe(store, args[1]);
    uint32_t length = matte_string_get_utf8_length(val);

    if (m->alloc <= m->size + length) {
        uint64_t prevAlloc = m->alloc;
        m->alloc = 10 + (m->alloc+length)*1.1;
        uint8_t * newBuffer = (uint8_t*)matte_allocate(m->alloc);
        memcpy(newBuffer, m->buffer, prevAlloc);
        matte_deallocate(m->buffer);
        m->buffer = newBuffer;
    }
    memcpy(m->buffer+m->size, matte_string_get_utf8_data(val), length);
    m->size+=length;
    return matte_store_new_value(store);
}


MATTE_EXT_FN(matte_ext__memory_buffer__append) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * mA = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);
    matteValue_t b = args[1];
    MatteMemoryBuffer * mB = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, b);




    if (mA->alloc < mA->size + mB->size) {
        uint64_t oldAlloc = mA->alloc;
        mA->alloc = mA->size + mB->size;
        uint8_t * newBuffer = (uint8_t*)matte_allocate(mA->alloc);
        memcpy(newBuffer, mA->buffer, oldAlloc);
        matte_deallocate(mA->buffer);
        mA->buffer = newBuffer;
    }
    memcpy(mA->buffer+mA->size, mB->buffer, mB->size);
    mA->size += mB->size;
    
    return matte_store_new_value(store);
}


MATTE_EXT_FN(matte_ext__memory_buffer__remove) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t from = matte_value_as_number(store, args[1]);
    uint64_t to   = matte_value_as_number(store, args[2]);

    if (from > m->size || to > m->size || from > to) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Remove call failed: Improper indices for buffer"));
        return matte_store_new_value(store);                   
    }


    memmove(m->buffer+from, m->buffer+to+1, m->size - (to+1));
    m->size-=(to-from)+1;
    return matte_store_new_value(store);
}


MATTE_EXT_FN(matte_ext__memory_buffer__get_size) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    matteValue_t out = matte_store_new_value(store);
    matte_value_into_number(store, &out, m->size);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__get_index) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t from = matte_value_as_number(store, args[1]);
    if (from >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not get value from buffer: index out of range."));
        return matte_store_new_value(store);                   

    }

    matteValue_t out = matte_store_new_value(store);
    matte_value_into_number(store, &out, m->buffer[from]);
    return out;    
}

MATTE_EXT_FN(matte_ext__memory_buffer__as_utf8) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    char * buf = (char*)matte_allocate(m->size+1);
    buf[m->size] = 0;
    memcpy(buf, m->buffer, m->size);
    
    matteString_t * outStr = matte_string_create_from_c_str("%s", buf);
    matte_deallocate(buf);
    
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_string(store, &out, outStr);
    matte_string_destroy(outStr);
    return out;
}

MATTE_EXT_FN(matte_ext__memory_buffer__set_index) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);

    uint64_t from = matte_value_as_number(store, args[1]);
    uint8_t  val  = matte_value_as_number(store, args[2]);
    if (from >= m->size) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not set value in buffer: index out of range."));
        return matte_store_new_value(store);                   
    }

    m->buffer[from] = val;
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_number(store, &out, m->buffer[from]);
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_ext__memory_buffer__read_primitive) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);
    uint64_t offset = matte_value_as_number(store, args[1]);


    int byteSize = 1;
    int primitive = matte_value_as_number(store, args[2]);

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
        return matte_store_new_value(store);                   
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


    matteValue_t out = matte_store_new_value(store);
    matte_value_into_number(store, &out, val);
    return out;    
}



MATTE_EXT_FN(matte_ext__memory_buffer__write_primitive) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = args[0];
    MatteMemoryBuffer * m = (MatteMemoryBuffer*)matte_value_object_get_userdata(store, a);
    uint64_t offset = matte_value_as_number(store, args[1]);
    

    int byteSize = 1;
    int primitive = matte_value_as_number(store, args[2]);

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
        return matte_store_new_value(store);                   
    }




    double val = matte_value_as_number(store, args[3]); // best we can do. for now.
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


    return matte_store_new_value(store);
}








static void matte_system__memorybuffer(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_create"),        0, matte_ext__memory_buffer__create,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_release"),       1, matte_ext__memory_buffer__release,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set_size"),      2, matte_ext__memory_buffer__set_size,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_copy"),          5, matte_ext__memory_buffer__copy,       NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set"),           4, matte_ext__memory_buffer__set,        NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_subset"),        3, matte_ext__memory_buffer__subset,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_append_byte"),   2, matte_ext__memory_buffer__append_byte,NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_append_utf8"),   2, matte_ext__memory_buffer__append_utf8,NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_append"),        2, matte_ext__memory_buffer__append     ,NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_remove"),        3, matte_ext__memory_buffer__remove,     NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_get_size"),      1, matte_ext__memory_buffer__get_size,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_get_index"),     2, matte_ext__memory_buffer__get_index,  NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_set_index"),     3, matte_ext__memory_buffer__set_index,  NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_as_utf8"),       1, matte_ext__memory_buffer__as_utf8,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_write_primitive"),4, matte_ext__memory_buffer__write_primitive,    NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::mbuffer_read_primitive"), 3, matte_ext__memory_buffer__read_primitive,    NULL);

 
}
