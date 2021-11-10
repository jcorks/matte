#include "../shared.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


static matteValue_t * initialArgs;
static uint32_t initialArgCount;



MATTE_EXT_FN(matte_cli__system_getargcount) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(&v, initialArgCount);
    return v;
}

MATTE_EXT_FN(matte_cli__system_getarg) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    if (matte_vm_pending_message(vm)) return v;
    
    if (index < initialArgCount) {
        matte_value_into_copy(&v, initialArgs[index]);
    }
    
    matte_value_into_number(&v, initialArgCount);
    return v;
}







#ifdef _POSIX_SOURCE
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>










#elif _WIN32

#endif



/*


*/

void matte_vm_add_system_symbols(matteVM_t * vm, char ** args, int argc) {
    initialArgCount = argc;
    initialArgs = calloc(argc, sizeof(matteValue_t));
    matteHeap_t * heap = matte_vm_get_heap(vm);
    uint32_t i;
    for(i = 0; i < initialArgCount; ++i) {
        matteString_t * str = matte_string_create_from_c_str(args[i]);
        initialArgs[i] = matte_heap_new_value(heap);
        matte_value_into_string(initialArgs+i, str);
        matte_string_destroy(str);
    }

    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getargcount"),   0, matte_cli__system_getargcount,   NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getarg"), 1, matte_cli__system_getarg, NULL);



    // stdlib only (system agnostic)
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_readstring"),   1, matte_cli__system_readstring, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_readbytes"),    1, matte_cli__system_readbytes, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_writestring"),  2, matte_cli__system_writestring, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_writebytes"),   2, matte_cli__system_writebytes, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_exit"),   1, matte_cli__system_exit, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_time"),   0, matte_cli__system_time, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_system"),   1, matte_cli__system_system, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_initRand"), 0, matte_cli__system_init_rand, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_nextRand"), 0, matte_cli__system_next_rand, NULL);


    // OS specific
    
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_sleepms"),   1, matte_cli__system_sleepms, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getticks"),    0, matte_cli__system_getticks, NULL);

    



    
    
}
