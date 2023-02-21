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
