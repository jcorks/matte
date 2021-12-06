
#include <stdlib.h>
#include <string.h>

#define MATTE_STRING_OBJECT_IDTAG 0xfeef5639

typedef struct {
    matteString_t * str;
    uint32_t idtag;
} MatteStringObject;


static void strobj_cleanup(void * userdata, void * fndata) {
    MatteStringObject * st = userdata;
    matte_string_destroy(st->str);
    free(st);
    
}

MATTE_EXT_FN(matte_core__string_create) {
    matteValue_t input = args[0];

    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_heap_new_value(heap);

    matte_value_into_new_object_ref(heap, &v);
    MatteStringObject * m = calloc(1, sizeof(MatteStringObject));
    m->idtag = MATTE_STRING_OBJECT_IDTAG;
    if (input.binID == MATTE_VALUE_TYPE_STRING) {
        m->str = matte_string_clone(matte_value_string_get_string_unsafe(heap, input));        
    } else {
        m->str = matte_string_create();
    }
    matte_value_object_set_native_finalizer(heap, v, strobj_cleanup, NULL);
    matte_value_object_set_userdata(heap, v, m);
    return v;
}

static MatteStringObject * get_strobj(matteVM_t * vm, matteValue_t v) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = matte_value_object_get_userdata(heap, v);
    if (!(st && st->idtag == MATTE_STRING_OBJECT_IDTAG)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Instance is not a string."));
        return NULL;    
    }
    return st;
}   


MATTE_EXT_FN(matte_core__string_get_length) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, matte_string_get_length(st->str));
    return out;
}

MATTE_EXT_FN(matte_core__string_set_length) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    
    uint32_t len = matte_value_as_number(heap, newlV);
    if (len < matte_string_get_length(st->str)) {
        matte_string_truncate(st->str, len);
    } else if (len > matte_string_get_length(st->str)) {
        uint32_t i;
        for(i = matte_string_get_length(st->str); i < len; ++i) {
            matte_string_append_char(st->str, ' ');
        }    
    }

    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_core__string_search) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * other = matte_value_string_get_string_unsafe(heap, newlV);
    matteValue_t out = matte_heap_new_value(heap);    
    
    uint32_t i, n;
    uint32_t len = matte_string_get_length(st->str);
    uint32_t lenOther = matte_string_get_length(other);
    if (!lenOther || lenOther > len) {
        matte_value_into_number(heap, &out, -1);
        return out;      
    }
    for(i = 0; i < len - (lenOther-1); ++i) {
        for(n = 0; n < lenOther && i+n < len; ++n) {
            if (matte_string_get_char(st->str, i+n) !=
                matte_string_get_char(other, n)) {
                break;
            }
        }
        
        if (n == lenOther) {
            matte_value_into_number(heap, &out, i);
            return out;
        }
    }
        
    matte_value_into_number(heap, &out, -1);
    return out;  
}

MATTE_EXT_FN(matte_core__string_replace) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * other = matte_value_string_get_string_unsafe(heap, newlV);

    newlV = args[2];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * newst = matte_value_string_get_string_unsafe(heap, newlV);


    matteValue_t out = matte_heap_new_value(heap);    
    
    uint32_t i, n;
    uint32_t len = matte_string_get_length(st->str);
    uint32_t lenOther = matte_string_get_length(other);
    uint32_t lenNew = matte_string_get_length(newst);

    uint32_t * newstBuffer = malloc(sizeof(uint32_t) * lenNew);
    for(n = 0; n < lenNew; ++n) {
        newstBuffer[n] = matte_string_get_char(newst, n);
    }


    for(i = 0; i < len - (lenOther-1); ++i) {
        for(n = 0; n < lenOther && i+n < len; ++n) {
            if (matte_string_get_char(st->str, i+n) !=
                matte_string_get_char(other, n)) {
                break;
            }
        }
        
        if (n == lenOther) { // found:
            matte_string_remove_n_chars(st->str, i, lenOther);
            if (i >= len - lenOther) {
                matte_string_concat(st->str, newst); 
            } else {
                matte_string_insert_n_chars(st->str, i, newstBuffer, lenNew); 
            }
            len = matte_string_get_length(st->str);
        }
    }
    free(newstBuffer);
        
    return out;  
}
    
    
MATTE_EXT_FN(matte_core__string_count) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * other = matte_value_string_get_string_unsafe(heap, newlV);
    matteValue_t out = matte_heap_new_value(heap);    
    
    uint32_t i, n;
    uint32_t len = matte_string_get_length(st->str);
    uint32_t lenOther = matte_string_get_length(other);
    uint32_t count = 0;
    for(i = 0; i < len - (lenOther-1); ++i) {
        for(n = 0; n < lenOther && i+n < len; ++n) {
            if (matte_string_get_char(st->str, i+n) !=
                matte_string_get_char(other, n)) {
                break;
            }
        }
        
        if (n == lenOther) {
            count++;
        }
    }
        
    matte_value_into_number(heap, &out, count);
    return out;  
}

    
MATTE_EXT_FN(matte_core__string_charcodeat) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t index = matte_value_as_number(heap, newlV);
    if (index >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }
    
    
    matteValue_t out = matte_heap_new_value(heap);    
    matte_value_into_number(heap, &out, matte_string_get_char(st->str, index));
    return out;  
}

MATTE_EXT_FN(matte_core__string_charat) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t index = matte_value_as_number(heap, newlV);
    if (index >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }
    
    
    matteValue_t out = matte_heap_new_value(heap);    
    matte_value_into_string(heap, &out, matte_string_get_substr(st->str, index, index));
    return out;  
}

MATTE_EXT_FN(matte_core__string_setcharcodeat) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t index = matte_value_as_number(heap, newlV);
    if (index >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }
    
    newlV = args[2];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t val = matte_value_as_number(heap, newlV);


    matte_string_set_char(st->str, index, val);
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_core__string_setcharat) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t index = matte_value_as_number(heap, newlV);
    if (index >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }
    
    newlV = args[2];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    
    const matteString_t * str = matte_value_string_get_string_unsafe(heap, newlV);
    if (matte_string_get_length(str) == 0) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument string is empty"));
        return matte_heap_new_value(heap);
    }
    matte_string_set_char(st->str, index, matte_string_get_char(str, 0));
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_core__string_append) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * str = matte_value_string_get_string_unsafe(heap, newlV);

    uint32_t i;
    uint32_t len = matte_string_get_length(str);
    for(i = 0; i < len; ++i) {
        matte_string_append_char(st->str, matte_string_get_char(str, i));
    }    

    return matte_heap_new_value(heap);    
}


MATTE_EXT_FN(matte_core__string_removechar) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);


    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }

    uint32_t index = matte_value_as_number(heap, newlV);
    if (index >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }

    matte_string_remove_n_chars(st->str, index, 1);
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_core__string_substr) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);


    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t indexFrom = matte_value_as_number(heap, newlV);
    if (indexFrom >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }

    newlV = args[2];
    if (!newlV.binID == MATTE_VALUE_TYPE_NUMBER) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a number"));
        return matte_heap_new_value(heap);
    }
    uint32_t indexTo = matte_value_as_number(heap, newlV);
    if (indexTo >= matte_string_get_length(st->str)) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "String index is out of bounds"));
        return matte_heap_new_value(heap);        
    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, matte_string_get_substr(st->str, indexFrom, indexTo));
    return out;    
}




MATTE_EXT_FN(matte_core__string_split) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t newlV = args[1];
    if (!newlV.binID == MATTE_VALUE_TYPE_STRING) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Argument is not a string"));
        return matte_heap_new_value(heap);
    }
    const matteString_t * other = matte_value_string_get_string_unsafe(heap, newlV);
    matteValue_t out = matte_heap_new_value(heap);    

    matteArray_t * arr = matte_array_create(sizeof(matteValue_t));
    
    uint32_t i, n;
    uint32_t len = matte_string_get_length(st->str);
    uint32_t lenOther = matte_string_get_length(other);
    uint32_t lastStart = 0;
    for(i = 0; i < len; ++i) {
        for(n = 0; n < lenOther && i+n < len; ++n) {
            if (matte_string_get_char(st->str, i+n) !=
                matte_string_get_char(other, n)) {
                break;
            }
        }
        
        if (n == lenOther) {
            if (i) {
                const matteString_t * subgood = matte_string_get_substr(st->str, lastStart, i-1);
                matteValue_t subv = matte_heap_new_value(heap);
                matte_value_into_string(heap, &subv, subgood);            
                matte_array_push(arr, subv);
            }
            i += lenOther-1;
            lastStart = i+1;
        }
    }
    if (n != lenOther) {
        const matteString_t * subgood = matte_string_get_substr(st->str, lastStart, len-1);
        matteValue_t subv = matte_heap_new_value(heap);
        matte_value_into_string(heap, &subv, subgood);            
        matte_array_push(arr, subv);
    }

    matte_value_into_new_object_array_ref(heap, &out, arr);
    matte_array_destroy(arr);
    return out;  
}


MATTE_EXT_FN(matte_core__string_get_string) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteStringObject * st = get_strobj(vm, args[0]);     
    if (!st) return matte_heap_new_value(heap);

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, st->str);
    return out;
}



static void matte_core__string(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_create"),     1, matte_core__string_create,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_get_length"), 1, matte_core__string_get_length,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_set_length"), 2, matte_core__string_set_length,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_search"),     2, matte_core__string_search, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_replace"),    3, matte_core__string_replace, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_count"),      2, matte_core__string_count, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_charcodeat"), 2, matte_core__string_charcodeat, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_charat"),     2, matte_core__string_charat, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_setcharcodeat"), 3, matte_core__string_setcharcodeat, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_setcharat"),  3, matte_core__string_setcharat, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_append"),     2, matte_core__string_append, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_removechar"), 2, matte_core__string_removechar, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_substr"),     3, matte_core__string_substr, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_split"),      2, matte_core__string_split, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::string_get_string"), 1, matte_core__string_get_string, NULL);

}
