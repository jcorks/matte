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

static int TESTID = 1;


static void test_string_utf8(matteVM_t * vm) {
    matteString_t * str = matte_string_create();
    assert(matte_string_get_length(str) == 0);
    assert(matte_string_get_byte_length(str) == 0);
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
    assert(matte_string_get_byte_length(str) == 0);
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


static void onError(const matteString_t * s, uint32_t line, uint32_t ch, void * userdata) {
    printf("TEST COMPILE FAILURE ON TEST: %d\n:", TESTID);
    printf("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    fflush(stdout);
    exit(1);
}

static void onErrorCatch(
    matteVM_t * vm, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t value, 
    void * data
) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, matte_value_object_access_string(heap, value, MATTE_VM_STR_CAST(vm, "detail"))));
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
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t a = matte_heap_new_value(heap);
    matte_value_into_number(heap, &a,
        matte_value_as_number(heap, args[0]) + 
        matte_value_as_number(heap, args[1])
    );
    return a;
}

int main() {
    uint32_t i = 0;
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

        matte_t * m = matte_create();
        matteVM_t * vm = matte_get_vm(m);
        matteHeap_t * heap = matte_vm_get_heap(vm);
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
        uint8_t * outBytes = matte_compiler_run(
            src,
            lenBytes,
            &outByteLen,
            onError,
            NULL
        );

        free(src);
        if (!outByteLen || !outBytes) {
            printf("Couldn't compile source %s\n", matte_string_get_c_str(infile));
            exit(1);
        }
        

        matteArray_t * arr = matte_bytecode_stubs_from_bytecode(heap, matte_vm_get_new_file_id(vm, infile), outBytes, outByteLen);
        matte_vm_add_stubs(vm, arr);
        matte_array_destroy(arr);
        free(outBytes);

        matteValue_t v = matte_vm_run_fileid(vm, i+1, matte_heap_new_value(matte_vm_get_heap(vm)), NULL);

        char * outstr = dump_string(matte_string_get_c_str(outfile));
        if (!outstr) {
            printf("Could not open output file %s.\n", matte_string_get_c_str(outfile));
            exit(1);
        }
        matteString_t * outputText = matte_string_create();
        matte_string_concat_printf(outputText, outstr);

        const matteString_t * resultText = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, v));
        if (!matte_string_test_eq(outputText, resultText)) {
            const matteString_t * str = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, v));
            printf("Test failed!!\nExpected output   : %s\nReal output       : %s\n", outstr, matte_string_get_c_str(str));
            exit(1);
        }
        printf("Pass. Cleaning up...");
        fflush(stdout);


        free(outstr);
        TESTID++;
        matte_string_destroy(outputText);
        matte_heap_recycle(heap, v);
        matte_destroy(m);
        printf("Done.\n");
        fflush(stdout);

    }
    matte_array_destroy(args);
    matte_string_destroy(infile);
    matte_string_destroy(outfile);
    printf("Tests pass.\n");
    return 0;
}
