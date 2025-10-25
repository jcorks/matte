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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include "../../matte_string.h"
#include "../../matte_table.h"

static uint32_t utf8_next_char(uint8_t ** source) {
    uint8_t * iter = *source;
    uint32_t val = (*source)[0];
    if (val < 128 && *iter) {
        val = (iter[0]) & 0x7F;
        (*source)++;
        return val;
    } else if (val < 224 && *iter && iter[1]) {
        val = ((iter[0] & 0x1F)<<6) + (iter[1] & 0x3F);
        (*source)+=2;
        return val;
    } else if (val < 240 && *iter && iter[1] && iter[2]) {
        val = ((iter[0] & 0x0F)<<12) + ((iter[1] & 0x3F)<<6) + (iter[2] & 0x3F);
        (*source)+=3;
        return val;
    } else if (*iter && iter[1] && iter[2] && iter[3]) {
        val = ((iter[0] & 0x7)<<18) + ((iter[1] & 0x3F)<<12) + ((iter[2] & 0x3F)<<6) + (iter[3] & 0x3F);
        (*source)+=4;
        return val;
    }
    return 0;
}

static void push_char_buffer(matteArray_t * buffer, uint32_t ch) {
    matte_array_push(buffer, ch);
}

static void push_cstr_buffer(matteArray_t * buffer, const char * str) {
    for(;;) {
        uint32_t next = utf8_next_char((uint8_t**)&str);
        if (next == 0) break;
        matte_array_push(buffer, next);
    }
}

static void push_cstr_buffer_cleaned(matteArray_t * buffer, const char * str) {
    for(;;) {
        uint32_t next = utf8_next_char((uint8_t**)&str);
        if (next == 0) break;
        
        if (next == '\\') {
            matte_array_push(buffer, next);  
        }
        
        if (next == '"') {
            uint32_t n = '\\';
            matte_array_push(buffer, n);   
        }
        matte_array_push(buffer, next);
    }
}


static void push_double_buffer(matteArray_t * buffer, double val) {
    static char tempstr[128];
    
    if (fabs(val - (int64_t)val) < DBL_EPSILON) {
        sprintf(tempstr, "%" PRId64 "", (int64_t)val);
    } else if (isnan(val)) {
        sprintf(tempstr, "null");
    } else {
        sprintf(tempstr, "%.15g", val);        
    }
    
    push_cstr_buffer(buffer, tempstr);
}

typedef struct {
    matteVM_t * vm;
    matteStore_t * store;
    matteString_t * lastNamedRef;
    matteTable_t * recur;
    matteArray_t * buffer;
} JSONEncodeData;

static int encode_value(JSONEncodeData * data, matteValue_t val) {
    matteStore_t * store = data->store;
    matteArray_t * buffer = data->buffer;
    switch(matte_value_type(val)) {
      case MATTE_VALUE_TYPE_BOOLEAN: 
        push_cstr_buffer(buffer, matte_value_as_boolean(store, val) == 1 ? "true" : "false");
        break;        
    
      case MATTE_VALUE_TYPE_NUMBER: 
        push_double_buffer(buffer, matte_value_get_number(val));
        break;

      case MATTE_VALUE_TYPE_STRING: {
        push_char_buffer(buffer, '"');
        push_cstr_buffer_cleaned(buffer, matte_string_get_c_str(matte_value_string_get_string_unsafe(store, val)));
        push_char_buffer(buffer, '"');
        break;
      }        
      case MATTE_VALUE_TYPE_EMPTY:
        push_cstr_buffer(buffer, "null");
        break;
        
      case MATTE_VALUE_TYPE_OBJECT: {
        if (matte_value_is_function(val)) {
            matteString_t * err = matte_string_create_from_c_str(
                "JSON encoding error: member \"%s\" is a function and is therefore unserializable.",
                matte_string_get_c_str(data->lastNamedRef)
            );
            matte_vm_raise_error_string(data->vm, err);
            matte_string_destroy(err);
            return 0;
        }
      
        matte_table_insert_by_uint(data->recur, val.value.id, (void*)0x1);
        // if is array
        if (matte_value_object_get_key_count(store, val) == matte_value_object_get_number_key_count(store, val)) {
            push_char_buffer(buffer, '[');
            uint32_t len = matte_value_object_get_key_count(store, val);
            uint32_t i;
            for(i = 0; i < len; ++i) {
                if (i != 0)
                    push_char_buffer(buffer, ',');

                matteValue_t * subval = matte_value_object_array_at_unsafe(store, val, i);

                matte_string_truncate(data->lastNamedRef, 0);
                matte_string_concat_printf(data->lastNamedRef, "[array index %d]", i);


                if (matte_value_type(*subval) == MATTE_VALUE_TYPE_OBJECT && matte_table_find_by_uint(data->recur, subval->value.id)) {
                    matteString_t * err = matte_string_create_from_c_str(
                        "JSON encoding error: circular dependency detected in %d index while trying to encode member \"%s\".",
                        (int) i,
                        matte_string_get_c_str(data->lastNamedRef)
                    );
                    matte_vm_raise_error_string(data->vm, err);
                    matte_string_destroy(err);
                    return 0;
                }
                if (!encode_value(data, *subval)) return 0;
            }
            
            push_char_buffer(buffer, ']');
        } else {
            matteValue_t keys = matte_value_object_keys(store, val);
            matte_value_object_push_lock(store, keys);
            
            uint32_t len = matte_value_object_get_key_count(store, keys);
            uint32_t i;
            push_char_buffer(buffer, '{');
            
            for(i = 0; i < len; ++i) {
                matteValue_t * key = matte_value_object_array_at_unsafe(store, keys, i);
                if (matte_value_type(*key) != MATTE_VALUE_TYPE_STRING) continue;

                if (i != 0)
                    push_char_buffer(buffer, ',');
                    
                    
                // key
                push_char_buffer(buffer, '"');
                push_cstr_buffer_cleaned(buffer, matte_string_get_c_str(matte_value_string_get_string_unsafe(store, *key)));
                push_char_buffer(buffer, '"');
                push_char_buffer(buffer, ':');


                // value;
                matteValue_t keyval = matte_value_object_access(store, val, *key, 1);
                matte_string_truncate(data->lastNamedRef, 0);
                matte_string_concat(data->lastNamedRef, matte_value_string_get_string_unsafe(store, *key));
                
                if (matte_value_type(keyval) == MATTE_VALUE_TYPE_OBJECT && matte_table_find_by_uint(data->recur, keyval.value.id)) {
                    matteString_t * err = matte_string_create_from_c_str(
                        "JSON encoding error: circular dependency detected; member \"%s\" was already encoded.",
                        (int) i,
                        matte_string_get_c_str(data->lastNamedRef)
                    );
                    matte_vm_raise_error_string(data->vm, err);
                    matte_string_destroy(err);
                    matte_value_object_pop_lock(store, keys);
                    return 0;
                }
                
                int ret = encode_value(data, keyval);
                matte_store_recycle(store, keyval);
                if (!ret) {
                    matte_value_object_pop_lock(store, keys);
                    return 0;
                }
            }
            matte_value_object_pop_lock(store, keys);
            push_char_buffer(buffer, '}');
        }
        break;
      }
      
      default : {
        matteString_t * err = matte_string_create_from_c_str(
            "JSON encoding error: member \"%s\" is an unknown value type.",
            matte_string_get_c_str(data->lastNamedRef)
        );
        matte_vm_raise_error_string(data->vm, err);
        matte_string_destroy(err);
        return 0;
      }
    }
    return 1;
}

matteString_t * matte_json_encode(matteVM_t * vm, matteValue_t object) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteArray_t * arr = matte_array_create(sizeof(uint32_t));
    matteTable_t * recur = matte_table_create_hash_pointer();
    
    JSONEncodeData data = {};
    data.store = store;
    data.vm = vm;
    data.recur = recur;
    data.lastNamedRef = matte_string_create_from_c_str("[toplevel object]");
    data.buffer = arr;
   
    
    encode_value(&data, object);    
    
    matte_string_destroy(data.lastNamedRef);
    matteString_t * str = matte_string_create_from_array_xfer(arr);
    matte_table_destroy(recur);
    return str;
}

MATTE_EXT_FN(matte_json__encode) {
    matteString_t * str = matte_json_encode(vm, args[0]);
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_string(store, &out, str);
    matte_string_destroy(str);
    return out;
}
typedef struct{
    int index;
    matteString_t * str;
} StringIter;

static uint32_t iter_peek(StringIter * iter, int index) {
    if (iter->index + index >= matte_string_get_length(iter->str)) return 0;
    return matte_string_get_char(iter->str, iter->index + index);
}

static void iter_trim_space(StringIter * iter) {
    while(iter->index < matte_string_get_length(iter->str) && isspace(matte_string_get_char(iter->str, iter->index))) 
        iter->index++;
}

matteValue_t decode_value(matteVM_t * vm, matteStore_t * store, StringIter * iter) {
    iter_trim_space(iter);
    matteValue_t out = matte_store_new_value(store);

    switch(iter_peek(iter, 0)) {
      case '"': {
        iter->index++;
        int count = 0;
        
        matteString_t * substr = matte_string_create();
        while(iter->index + count < matte_string_get_length(iter->str)) {
            switch(iter_peek(iter, count)) {
              case '\\':
                count ++;
                
                switch(iter_peek(iter, count)) {
                  case 'n' : matte_string_append_char(substr, '\n'); break;
                  case 'b' : matte_string_append_char(substr, '\b'); break;
                  case 'r' : matte_string_append_char(substr, '\r'); break;
                  case 't' : matte_string_append_char(substr, '\t'); break;
                  case '\\': matte_string_append_char(substr, '\\'); break;
                  case '"': matte_string_append_char(substr, '"'); break;
                  
                }
                count++;
                break;
                
              case '"':
                break;
                
              default:
                matte_string_append_char(substr, iter_peek(iter, count));
                count ++;
            }
            
            if (iter_peek(iter, count) == '"') break;
        }
        
        matte_value_into_string(store, &out, substr);
        matte_string_destroy(substr);
        iter->index += count+1;
        
        break;
      }
      case 'n': {
        if (iter_peek(iter, 1) == 'u' &&
            iter_peek(iter, 2) == 'l' &&
            iter_peek(iter, 3) == 'l') {
            iter->index += 4;
            
        } else {
            matte_vm_raise_error_cstring(vm, "Unrecognized token (expected null)");
        }
        break;
      }
      
      
      case 't': {
        if (iter_peek(iter, 1) == 'r' &&
            iter_peek(iter, 2) == 'u' &&
            iter_peek(iter, 3) == 'e') {
            iter->index += 4;

            matte_value_into_boolean(store, &out, 1);            
        } else {
            matte_vm_raise_error_cstring(vm, "Unrecognized token (expected true)");
        }
        break;
      }

      case 'f': {
        if (iter_peek(iter, 1) == 'a' &&
            iter_peek(iter, 2) == 'l' &&
            iter_peek(iter, 3) == 's' &&
            iter_peek(iter, 4) == 'e') {
            iter->index += 5;

            matte_value_into_boolean(store, &out, 0);            
        } else {
            matte_vm_raise_error_cstring(vm, "Unrecognized token (expected true)");
        }
        break;
      }
      
      
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-': {
        int start = iter->index;
        int count = 1;
        int done = 0;
        while(!done) {
            if (count + start >= matte_string_get_length(iter->str)) break;
            int p = iter_peek(iter, count);
            
            switch(p) {
              case 'e':
              case '.':
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
              case '-': {
                count ++;
                break;
              };
              
              default: 
                done = 1;
                break;
            }
        }
        
        const matteString_t * numstr = matte_string_get_substr(iter->str, start, start+count-1);
        double num = strtod(matte_string_get_c_str(numstr), NULL);
        matte_value_into_number(store, &out, num);
        iter->index += matte_string_get_length(numstr);
        break;
      }
        
        
      case '{': {
        matte_value_into_new_object_ref(store, &out);
        iter->index++;
        iter_trim_space(iter);

        // empty object
        if (iter_peek(iter, 0) == '}') {
            iter->index++;
            break;
        }
      
        while(iter->index <= matte_string_get_length(iter->str)) {
            matteValue_t key = decode_value(vm, store, iter);
            iter_trim_space(iter);
            
            // skip :
            iter->index++;

            matteValue_t value = decode_value(vm, store, iter);
            iter_trim_space(iter);
            
            matte_value_object_set(store, out, key, value, 1);
            
            
            int next = iter_peek(iter, 0);
            switch(next) {
              case ',': 
              case '}': 
                break;
                
              default:
                matte_vm_raise_error_cstring(vm, "Unknown character while parsing object.");
                break;
            }
                    
            iter->index++;
            if (next == '}')
                break;
        }    
        break;  
      }
        
      // array
      case '[': {
        iter->index++;
        iter_trim_space(iter);

        // empty object
        if (iter_peek(iter, 0) == ']') {
            iter->index++;
            matte_value_into_new_object_ref(store, &out);
            break;
        }

        matteArray_t * arr = matte_array_create(sizeof(matteValue_t));

        while(iter->index < matte_string_get_length(iter->str)) {


            matteValue_t next = decode_value(vm, store, iter);
            iter_trim_space(iter);
            matte_array_push(arr, next);

            int ch = iter_peek(iter, 0);
            switch(ch) {
              case ',': 
              case ']': 
                break;
                
              default:
                matte_vm_raise_error_cstring(vm, "Unknown character while parsing array.");
                break;
            }
                    
            iter->index++;
            if (ch == ']')
                break;
        }

        matte_value_into_new_object_array_ref(store, &out, arr);
        matte_array_destroy(arr);
        break;      
      }
      default:
        matte_vm_raise_error_cstring(vm, "Could not parse value: unknown character.");

    }

    return out;
}

matteValue_t matte_json_decode(matteVM_t * vm, const matteString_t * str) {
    matteStore_t * store = matte_vm_get_store(vm);
    StringIter iter;
    iter.str = (matteString_t*)str;
    iter.index = 0;
    return decode_value(vm, store, &iter);
}

MATTE_EXT_FN(matte_json__decode) {
    matteStore_t * store = matte_vm_get_store(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(store, args[0]);   
    return matte_json_decode(vm, str);
}


void matte_system__json(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::json_encode"),   1, matte_json__encode,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::json_decode"),   1, matte_json__decode,   NULL);

}
