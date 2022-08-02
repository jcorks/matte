#include "shared.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
void * dump_bytes(const char * filename, uint32_t * len) {
    FILE * f = fopen(filename, "rb");
    if (!f) {
        printf("Could not open input file %s\n", filename);
        exit(1);    
    }
    char chunk[2048];
    int chunkSize;
    *len = 0;    
    while(chunkSize = (fread(chunk, 1, 2048, f))) *len += chunkSize;
    fseek(f, 0, SEEK_SET);


    void * out = malloc(*len);
    uint32_t iter = 0;
    while(chunkSize = (fread(chunk, 1, 2048, f))) {
        memcpy(out+iter, chunk, chunkSize);
        iter += chunkSize;
    }
    fclose(f);
    return out;
}


matteValue_t parse_parameter_line(matteVM_t * vm, const char * line) {
    uint32_t maxlen = strlen(line)+1;
    char ** args = malloc(maxlen * sizeof(char*));
    uint32_t ct = 0;
    const char * iter = line;
    const char * iterstart = line;
    while(*iter) {
        if (isspace(*iter)) {
            if (iter != iterstart) {
                char * str = malloc(maxlen);
                memcpy(str, iterstart, iter - iterstart);
                str[iter - iterstart] = 0;
                args[ct++] = str;
            }
            while(*iter && isspace(*iter)) iter++;
            iterstart = iter;
        } else {
            iter++;
        }
    }
    if (iter != iterstart) {
        char * str = malloc(maxlen);
        memcpy(str, iterstart, iter - iterstart);
        str[iter - iterstart] = 0;
        args[ct++] = str;
    }    
    
    matteValue_t out = parse_parameters(vm, args, ct);
    
    uint32_t i;
    for(i = 0; i < ct; ++i) {
        free(args[i]);
    }
    free(args);
    return out;
}

matteValue_t parse_parameters(matteVM_t * vm, char ** args, uint32_t count) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &v);

    uint32_t i = 0;
    uint32_t n = 0;

    uint32_t len;
    matteString_t * iter = matte_string_create();
    matteString_t * key = NULL;
    int isName = 1;
    for(n = 0; n < count; ++n) {
        matteString_t * str = matte_string_create();
        matte_string_concat_printf(str, "%s", args[n]);
        len = matte_string_get_length(str);

        for(i = 0; i < len; ++i) {
          switch(matte_string_get_char(str, i)) {
            case ':':
                if (isName == 1 && matte_string_get_length(iter)) {
                    isName = 0;
                    if (!key) {
                        key = iter;
                        iter = matte_string_create();                    
                    }                    break;
                }
            default:
                matte_string_append_char(iter, matte_string_get_char(str, i));
            }
        }

        if (isName == 0) {
            matteValue_t heapKey = matte_heap_new_value(heap);
            matteValue_t heapVal = matte_heap_new_value(heap);

            matte_value_into_string(heap, &heapKey, key);
            matte_value_into_string(heap, &heapVal, iter);

            matte_value_object_set(heap, v, heapKey, heapVal, 1);
            matte_string_destroy(key);
            matte_string_clear(iter);
            key = NULL;
            isName = 1;
        }
        matte_string_destroy(str);

    }
    return v;
}
