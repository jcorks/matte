#include "../system.h"



MATTE_EXT_FN(matte_consoleio__clear) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    printf("\x1b[2J");
    fflush(stdout);
    return matte_heap_new_value(heap);
}




#define GETLINE_SIZE 4096

MATTE_EXT_FN(matte_consoleio__getline) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    char * buffer = malloc(GETLINE_SIZE+1);
    buffer[0] = 0;

    fgets(buffer, GETLINE_SIZE, stdin);

    matteValue_t v =  matte_heap_new_value(heap);
    matte_value_into_string(&v, MATTE_VM_STR_CAST(vm, buffer));
    free(buffer);
    return v;    
}




MATTE_EXT_FN(matte_consoleio__print) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) return matte_heap_new_value(heap);

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (str) {
        printf("%s", matte_string_get_c_str(str));
        fflush(stdout);
    }
    return matte_heap_new_value(heap);
}

static void matte_system__consoleio(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_print"),   1, matte_consoleio__print,   NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_getline"), 0, matte_consoleio__getline, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_clear"),   0, matte_consoleio__clear, NULL);
}

