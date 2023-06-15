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
#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include "../src/matte_array.h"
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int TESTID = 48;



int64_t BYTES_USED = 0;
uint64_t ALLOCS = 0;
uint64_t FREES = 0;
uint64_t PEAK_USAGE = 0;

void * test_allocator(uint64_t size) {
    BYTES_USED += size;
    ALLOCS++;
    if (BYTES_USED > PEAK_USAGE)
        PEAK_USAGE = BYTES_USED;
    
    uint8_t * buffer = malloc(size + sizeof(uint64_t));
    memcpy(buffer, &size, sizeof(uint32_t));
    return buffer + sizeof(uint64_t);
}

void test_deallocator(void * buffer) {
    uint8_t * realBuffer = ((uint8_t*)buffer) - sizeof(uint64_t);
    uint32_t size = 0;
    memcpy(&size, realBuffer, sizeof(uint32_t));
    
    BYTES_USED -= size;
    FREES++;
    free(realBuffer);
}






static void test_string_utf8(matteVM_t * vm) {
    matteString_t * str = matte_string_create();
    assert(matte_string_get_length(str) == 0);
    assert(matte_string_get_utf8_length(str) == 0);
    assert(!strcmp(matte_string_get_c_str(str), ""));
    
    matte_string_concat_printf(str, "hello%d안녕world𐍈ㄅㄞˇ!!!", 4);
    assert(matte_string_get_length(str) == 20);
    assert(!strcmp(matte_string_get_c_str(str), "hello4안녕world𐍈ㄅㄞˇ!!!"));
    assert(matte_string_get_char(str, 4) == 'o');
    
    assert(matte_string_test_eq(str, MATTE_VM_STR_CAST(vm, "hello4안녕world𐍈ㄅㄞˇ!!!")));
    matte_string_set_char(str, 4, 'O');
    matte_string_append_char(str, 'i');
    assert(!matte_string_compare(str, MATTE_VM_STR_CAST(vm, "hellO4안녕world𐍈ㄅㄞˇ!!!i")));
    assert(matte_string_test_contains(str, MATTE_VM_STR_CAST(vm, "4안녕world𐍈")));
    assert(!matte_string_test_contains(str, MATTE_VM_STR_CAST(vm, "o4안녕woild𐍈ㄅㄞˇ")));

    matteString_t * str1 = matte_string_create_from_c_str("%s", matte_string_get_c_str(str));
    matteString_t * str2 = matte_string_clone(str1);
    matteString_t * str3 = matte_string_create();
    matte_string_set(str3, str); 
    
    assert(!matte_string_compare(str1, MATTE_VM_STR_CAST(vm, "hellO4안녕world𐍈ㄅㄞˇ!!!i")));
    assert(!strcmp(matte_string_get_c_str(str3), "hellO4안녕world𐍈ㄅㄞˇ!!!i"));
    assert(matte_string_test_eq(str3, str2));    
    
    matte_string_concat(str1, str2);
    assert(matte_string_compare(str1, str2) > 0);
    const matteString_t * subst = matte_string_get_substr(
        str,
        5,
        matte_string_get_length(str)-1
    );
    matte_string_concat_printf(str2, "hellO%s", matte_string_get_c_str(subst));
    assert(matte_string_test_eq(str1, str2)); 
    assert(!matte_string_test_eq(str, str1));

    matte_string_destroy(str);
    matte_string_destroy(str1);
    matte_string_destroy(str2);
    matte_string_destroy(str3);
}


static void test_string(matteVM_t * vm) {
    matteString_t * str = matte_string_create();
    assert(matte_string_get_length(str) == 0);
    assert(matte_string_get_utf8_length(str) == 0);
    assert(!strcmp(matte_string_get_c_str(str), ""));
    
    matte_string_concat_printf(str, "hello%dworld!!!", 4);
    assert(!strcmp(matte_string_get_c_str(str), "hello4world!!!"));
    assert(matte_string_get_char(str, 4) == 'o');
    
    assert(matte_string_test_eq(str, MATTE_VM_STR_CAST(vm, "hello4world!!!")));
    matte_string_set_char(str, 4, 'O');
    matte_string_append_char(str, 'i');
    assert(!matte_string_compare(str, MATTE_VM_STR_CAST(vm, "hellO4world!!!i")));
    assert(matte_string_test_contains(str, MATTE_VM_STR_CAST(vm, "4world")));
    assert(!matte_string_test_contains(str, MATTE_VM_STR_CAST(vm, "o4woild")));

    matteString_t * str1 = matte_string_create_from_c_str("%s", matte_string_get_c_str(str));
    matteString_t * str2 = matte_string_clone(str1);
    matteString_t * str3 = matte_string_create();
    matte_string_set(str3, str);
    
    assert(!matte_string_compare(str1, MATTE_VM_STR_CAST(vm, "hellO4world!!!i")));
    assert(!strcmp(matte_string_get_c_str(str3), "hellO4world!!!i"));
    assert(matte_string_test_eq(str3, str2));    
    
    matte_string_concat(str1, str2);
    assert(matte_string_compare(str1, str2) > 0);
    const matteString_t * subst = matte_string_get_substr(
        str,
        5,
        matte_string_get_length(str)-1
    );
    matte_string_concat_printf(str2, "hellO%s", matte_string_get_c_str(subst));
    assert(matte_string_test_eq(str1, str2)); 
    assert(!matte_string_test_eq(str, str1));

    matte_string_destroy(str);
    matte_string_destroy(str1);
    matte_string_destroy(str2);
    matte_string_destroy(str3);
}




static void onErrorCatch(
    matteVM_t * vm, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t value, 
    void * data
) {
    matteStore_t * store = matte_vm_get_store(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, matte_value_object_access_string(store, value, MATTE_VM_STR_CAST(vm, "detail"))));
    printf("TEST RAISED AN ERROR WHILE RUNNING:\n%s\n", str ? matte_string_get_c_str(str) : "(null)");
    printf("(file %s, line %d)\n", matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), lineNumber);
    uint32_t stacksize = matte_vm_get_stackframe_size(vm);
    printf("Callstack: \n");
    uint32_t i;
    uint32_t count;
    for(i = 0; i < stacksize; ++i) {
        matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, i);
        const matteString_t * str = matte_vm_get_script_name_by_id(
            vm, 
            matte_bytecode_stub_get_file_id(frame.stub)
        );

        printf("(@%d, file %s, line %d)\n", 
            i,
            str ? matte_string_get_c_str(str) : "???",
            (matte_bytecode_stub_get_instructions(frame.stub, &count))[frame.pc].lineNumber
        );
    }

    exit(1);

}

void * dump_bytes(const char * filename, uint32_t * len) {
    FILE * f = fopen(filename, "rb");
    if (!f) {
        return NULL;
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

char * dump_string(const char * filename) {
    uint32_t len;
    FILE * f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }
    char chunk[2048];
    int chunkSize;
    len = 0;    
    while(chunkSize = (fread(chunk, 1, 2048, f))) len += chunkSize;
    fseek(f, 0, SEEK_SET);


    char * out = malloc(len+1);
    out[len] = 0;
    uint32_t iter = 0;
    while(chunkSize = (fread(chunk, 1, 2048, f))) {
        memcpy(out+iter, chunk, chunkSize);
        iter += chunkSize;
    }

    char * outReal = malloc(len+1);
    sscanf(out, "%s", outReal);
    free(out);
    fclose(f);
    return outReal;
}


static matteValue_t test_external_function(
    matteVM_t * vm,
    matteValue_t fn, 
    const matteValue_t * args,
    void * data
) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t a = matte_store_new_value(store);
    matte_value_into_number(store, &a,
        matte_value_as_number(store, args[0]) + 
        matte_value_as_number(store, args[1])
    );
    return a;
}

int main() {
    uint32_t i = 0;
    
    matte_set_allocator(test_allocator, test_deallocator);
    
    matte_t * m = matte_create();
    test_string(matte_get_vm(m));
    test_string_utf8(matte_get_vm(m));
    matte_destroy(m);
    m = NULL;
    
    matteString_t * infile = matte_string_create();
    matteString_t * outfile = matte_string_create();


    int testNo = 0;
    matteArray_t * args = matte_array_create(sizeof(matteValue_t));  
    for(;;) {


        matte_string_clear(infile);
        matte_string_clear(outfile);
        matte_string_concat_printf(infile, "test%d.mt", TESTID);
        matte_string_concat_printf(outfile, "test%d.out", TESTID);

        uint32_t lenBytes;
        uint8_t * src = dump_bytes(matte_string_get_c_str(infile), &lenBytes);
        if (!src) break;
        
        char * srcstr = malloc(lenBytes+1);
        memcpy(srcstr, src, lenBytes);
        srcstr[lenBytes] = 0;
        
        free(src);
        matte_t * m = matte_create();
        matteVM_t * vm = matte_get_vm(m);
        matte_set_importer(m, NULL, NULL);
        matteStore_t * store = matte_vm_get_store(vm);
        const matteString_t * externalNames[] = {
            MATTE_VM_STR_CAST(vm, "a"),
            MATTE_VM_STR_CAST(vm, "b")
        };
        matteArray_t temp = MATTE_ARRAY_CAST(externalNames, matteString_t *, 2);
        matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "external_test!"), &temp, test_external_function, NULL);
        matte_vm_set_unhandled_callback(vm, onErrorCatch, NULL);


        if (!lenBytes) {
            printf("Couldn't open source %s\n", matte_string_get_c_str(infile));
            exit(1);
        }
        printf("Running test %s...", matte_string_get_c_str(infile));
        fflush(stdout);
        uint32_t outByteLen;
        uint8_t * outBytes = matte_compile_source(
            m,
            &outByteLen,
            srcstr
        );

        free(srcstr);
        if (!outByteLen || !outBytes) {
            printf("Couldn't compile source %s\n", matte_string_get_c_str(infile));
            exit(1);
        }
        
        uint32_t fileid = matte_vm_get_new_file_id(vm, infile);
        matteArray_t * arr = matte_bytecode_stubs_from_bytecode(store, fileid, outBytes, outByteLen);
        matte_vm_add_stubs(vm, arr);
        matte_array_destroy(arr);
        matte_deallocate(outBytes);

        matteValue_t v = matte_vm_run_fileid(vm, fileid, matte_store_new_value(matte_vm_get_store(vm)));

        char * outstr = dump_string(matte_string_get_c_str(outfile));
        if (!outstr) {
            printf("Could not open output file %s.\n", matte_string_get_c_str(outfile));
            exit(1);
        }
        matteString_t * outputText = matte_string_create();
        matte_string_concat_printf(outputText, outstr);

        const matteString_t * resultText = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, v));
        if (!matte_string_test_eq(outputText, resultText)) {
            const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, v));
            printf("Test failed!!\nExpected output   : %s\nReal output       : %s\n", outstr, matte_string_get_c_str(str));
            exit(1);
        }
        printf("Pass. Cleaning up...");
        fflush(stdout);


        free(outstr);
        TESTID++;
        matte_string_destroy(outputText);
        matte_destroy(m);
        printf("Done.\n");
        fflush(stdout);

    }
    matte_array_destroy(args);
    matte_string_destroy(infile);
    matte_string_destroy(outfile);
    printf("Tests pass.\n");
    
    printf("Memory report:\n");
    printf("Peak usage   : %.2f KB\n", ((double)PEAK_USAGE) / 1024);
    printf("Bytes leaked : %.2f KB\n", ((double)BYTES_USED) / 1024);
    
    
    return 0;
}
