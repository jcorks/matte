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
#include <windows.h>
#include <conio.h>
#include <ctype.h>

MATTE_EXT_FN(matte_consoleio__clear) {
    matteStore_t * store = matte_vm_get_store(vm);
    printf("\x1b[3J");
    printf("\x1b[0;0H");
    fflush(stdout);
    return matte_store_new_value(store);
}




#define GETLINE_SIZE 4096

MATTE_EXT_FN(matte_consoleio__getline) {
    matteStore_t * store = matte_vm_get_store(vm);

    char * buffer = (char*)malloc(GETLINE_SIZE+1);
    buffer[0] = 0;

    fgets(buffer, GETLINE_SIZE, stdin);

    matteValue_t v =  matte_store_new_value(store);
    matte_value_into_string(store, &v, MATTE_VM_STR_CAST(vm, buffer));
    free(buffer);
    return v;    
}



MATTE_EXT_FN(matte_consoleio__getch) {
    matteStore_t * store = matte_vm_get_store(vm);

    int nonblocking = 0;
    if (matte_value_as_boolean(store, args[0])) {
        nonblocking = TRUE;
    }
    char cstr[2] = {};
    cstr[1] = 0;
    if (nonblocking) {
        if (_kbhit())
            cstr[0] = _getch();
    } else {
        cstr[0] = _getch();
    }
    matteValue_t v =  matte_store_new_value(store);
    if (cstr[0] != 0) {
        matte_value_into_string(store, &v, MATTE_VM_STR_CAST(vm, cstr));
    }
    return v;   
}



MATTE_EXT_FN(matte_consoleio__print) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (str) {
        printf("%s", matte_string_get_c_str(str));
        fflush(stdout);
    }
    return matte_store_new_value(store);
}

static void matte_system__consoleio(matteVM_t * vm) {
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_print"),   1, matte_consoleio__print,   NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_getline"), 0, matte_consoleio__getline, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_clear"),   0, matte_consoleio__clear, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::consoleio_getch"),   1, matte_consoleio__getch, NULL);
}

