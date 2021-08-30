#include "../shared.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static matteValue_t matte_cli__system_print(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) return matte_heap_new_value(heap);

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (str) {
        printf("%s", matte_string_get_c_str(str));
        fflush(stdout);
    }
    return matte_heap_new_value(heap);
}



#define GETLINE_SIZE 4096

static matteValue_t matte_cli__system_getline(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    char * buffer = malloc(GETLINE_SIZE+1);
    buffer[0] = 0;

    fgets(buffer, GETLINE_SIZE, stdin);

    matteValue_t v =  matte_heap_new_value(heap);
    matte_value_into_string(&v, MATTE_STR_CAST(buffer));
    free(buffer);
    return v;    
}







void matte_vm_add_system_symbols(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_print"),   1, matte_cli__system_print,   NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getline"), 0, matte_cli__system_getline, NULL);


}