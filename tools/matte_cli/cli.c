#include "../../src/matte.h"
#include "../../src/matte_vm.h"
#include "../../src/matte_bytecode_stub.h"
#include "../../src/matte_array.h"
#include "../../src/matte_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char ** args) {
    if (argc < 3) {
        printf("Current usage: matte --bytecode [bytecode binary] [args...]\n");
        exit(1);
    }
    matte_t * m = matte_create();
    
    if (!strcmp(args[1], "--bytecode")) {
        matteVM_t * vm = matte_get_vm(m);
        uint32_t len;
        void * data = dump_bytes(args[2], &len);
        
        
        matte_vm_add_stubs(vm, matte_bytecode_stubs_from_bytecode(data, len));

        matteArray_t * arr = matte_array_create(sizeof(matteValue_t));        
        uint32_t i;
        for(i = 3; i < argc; ++i) {
            matteValue_t v = matte_heap_new_value(matte_vm_get_heap(vm));
            matte_value_into_string(&v, MATTE_STR_CAST(args[i]));
            matte_array_push(arr, v);
        }
        
        // for now hardcode fileid 1
        matteValue_t v = matte_vm_run_script(vm, 1, arr);
        printf("MatteVM run complete.\nResult: %s\n", matte_string_get_c_str(matte_value_as_string(v)));

        return 0;
        
    }
}
