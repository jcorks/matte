#ifndef H__MATTE_SYSTEM_INCLUDED
#define H__MATTE_SYSTEM_INCLUDED

#include "../matte_array.h"
#include "../matte_heap.h"
#include "../matte_vm.h"

#define MATTE_EXT_FN(__T__) static matteValue_t __T__(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData)
void matte_bind_native_functions();

#endif
