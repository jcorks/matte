#include "matte.h"
#include "matte_vm.h"
#include <stdlib.h>
struct matte_t {
    matteVM_t * vm;
    void * userdata;
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
    matte_vm_destroy(m->vm);
    free(m);
}

void * matte_get_user_data(matte_t * m) {
    return m->userdata;
}

void matte_set_user_data(matte_t * m, void * d) {
    m->userdata = d;
}