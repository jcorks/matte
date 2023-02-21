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

static void encode_value__clean_string(matteString_t * str) {
    uint32_t len = matte_string_get_length(str);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        if (matte_string_get_char(str, i) == '\\') {
            uint32_t ch = '\\';
            matte_string_insert_n_chars(str, i, &ch, 1);
            len++;
            i++;
        } 
    }

    for(i = 0; i < len; ++i) {
        if (matte_string_get_char(str, i) == '"') {
            uint32_t ch = '\\';
            matte_string_insert_n_chars(str, i, &ch, 1);
            len++;
            i++;
        } 
    }
}

static matteString_t * encode_value(matteHeap_t * heap, matteValue_t val) {
    switch(val.binID) {
      case MATTE_VALUE_TYPE_BOOLEAN: 
        return matte_string_create_from_c_str("%s", matte_value_as_boolean(heap, val) == 1 ? "true" : "false");
        
    
      case MATTE_VALUE_TYPE_NUMBER: 
        if (fabs(val.value.number - (int64_t)val.value.number) < DBL_EPSILON) {
            return matte_string_create_from_c_str("%"PRId64"", (int64_t)val.value.number);                
        } else {
            return matte_string_create_from_c_str("%.15g", val.value.number);        
        }

      case MATTE_VALUE_TYPE_STRING: {
        matteString_t * clean = matte_string_clone(matte_value_string_get_string_unsafe(heap, val));
        encode_value__clean_string(clean);
        matteString_t * out = matte_string_create_from_c_str("\"%s\"", matte_string_get_c_str(clean));
        matte_string_destroy(clean);
        return out;
      }        
      case MATTE_VALUE_TYPE_EMPTY:
        return matte_string_create_from_c_str("null");
        
      case MATTE_VALUE_TYPE_OBJECT: {
        matteString_t * out = matte_string_create();
        // if is array
        if (matte_value_object_get_key_count(heap, val) == matte_value_object_get_number_key_count(heap, val)) {
            matte_string_append_char(out, '[');
            
            uint32_t len = matte_value_object_get_key_count(heap, val);
            uint32_t i;
            for(i = 0; i < len; ++i) {
                if (i != 0)
                    matte_string_append_char(out, ',');

                matteValue_t * subval = matte_value_object_array_at_unsafe(heap, val, i);
                matteString_t * substr = encode_value(heap, *subval);
                matte_string_concat(out, substr);
                matte_string_destroy(substr);
            }
            
            matte_string_append_char(out, ']');            
        } else {
            matteValue_t keys = matte_value_object_keys(heap, val);
            
            uint32_t len = matte_value_object_get_key_count(heap, keys);
            uint32_t i;
            matte_string_append_char(out, '{');
            
            for(i = 0; i < len; ++i) {
                matteValue_t * key = matte_value_object_array_at_unsafe(heap, keys, i);
                if (key->binID != MATTE_VALUE_TYPE_STRING) continue;

                if (i != 0)
                    matte_string_append_char(out, ',');
                    
                    
                // key
                matteString_t * keystr = matte_string_clone(matte_value_string_get_string_unsafe(heap, *key));
                encode_value__clean_string(keystr);
                matte_string_concat_printf(
                    out,
                    "\"%s\"", matte_string_get_c_str(keystr)
                );                
                matte_string_destroy(keystr);
                
                
                matte_string_append_char(out, ':');


                // value;
                matteValue_t keyval = matte_value_object_access(heap, val, *key, 1);
                matteString_t * keyvalstr = encode_value(heap, keyval);
                matte_string_concat(out, keyvalstr);
                matte_string_destroy(keyvalstr);
            }
            matte_string_append_char(out, '}');
        }
        return out;      
      }
        
    }
    
    return matte_string_create_from_c_str("");
}

MATTE_EXT_FN(matte_json__encode) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteString_t * str = encode_value(heap, args[0]);   
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, str);
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

matteValue_t decode_value(matteVM_t * vm, matteHeap_t * heap, StringIter * iter) {
    iter_trim_space(iter);
    matteValue_t out = matte_heap_new_value(heap);

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
        
        matte_value_into_string(heap, &out, substr);
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

            matte_value_into_boolean(heap, &out, 1);            
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

            matte_value_into_boolean(heap, &out, 0);            
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
        matte_value_into_number(heap, &out, num);
        iter->index += matte_string_get_length(numstr);
        break;
      }
        
        
      case '{': {
        matte_value_into_new_object_ref(heap, &out);
        iter->index++;
        iter_trim_space(iter);

        // empty object
        if (iter_peek(iter, 0) == '}') {
            iter->index++;
            break;
        }
      
        while(iter->index <= matte_string_get_length(iter->str)) {
            matteValue_t key = decode_value(vm, heap, iter);
            iter_trim_space(iter);
            
            // skip :
            iter->index++;

            matteValue_t value = decode_value(vm, heap, iter);
            iter_trim_space(iter);
            
            matte_value_object_set(heap, out, key, value, 1);
            
            
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
            matte_value_into_new_object_ref(heap, &out);
            break;
        }

        matteArray_t * arr = matte_array_create(sizeof(matteValue_t));

        while(iter->index < matte_string_get_length(iter->str)) {


            matteValue_t next = decode_value(vm, heap, iter);
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

        matte_value_into_new_object_array_ref(heap, &out, arr);
        matte_array_destroy(arr);
        break;      
      }
      default:
        matte_vm_raise_error_cstring(vm, "Could not parse value: unknown character.");

    }

    return out;
}

MATTE_EXT_FN(matte_json__decode) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(heap, args[0]);   
    StringIter iter;
    iter.str = str;
    iter.index = 0;
    matteValue_t out = decode_value(vm, heap, &iter);
    return out;
}


void matte_system__json(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::json_encode"),   1, matte_json__encode,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::json_decode"),   1, matte_json__decode,   NULL);

}
