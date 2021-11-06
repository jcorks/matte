#ifndef H_MATTE_EXT_INCLUDED
#define H_MATTE_EXT_INCLUDED

#include "../matte_array.h"
#include "../matte_heap.h"
#include "../matte_vm.h"

#define MATTE_EXT_FN(__T__) static matteValue_t __T__(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData)


#endif
