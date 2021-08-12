#include "matte.h"
#include "matte_vm.h"
#include <stdlib.h>
struct matte_t {
    matteVM_t * vm;
};


matte_t * matte_create() {
    matte_t * m = calloc(1, sizeof(matte_t));
    m->vm = matte_vm_create();
    return m;
}


matteVM_t * matte_get_vm(matte_t * m) {
    return m->vm;
}

void matte_destroy(matte_t * m) {
    // cleanup here.
}