#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include "../src/matte_array.h"
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int TESTID = 1;

static void onError(const matteString_t * s, uint32_t line, uint32_t ch) {
    printf("TEST COMPILE FAILURE ON TEST: %d\n:", TESTID);
    printf("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    fflush(stdout);
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
    fclose(f);
    return out;
}


int main() {
    uint32_t i;
    


    matteString_t * infile = matte_string_create();
    matteString_t * outfile = matte_string_create();

    int testNo = 0;
    matteArray_t * args = matte_array_create(sizeof(matteValue_t));  
    for(;;) {
        matte_t * m = matte_create();
        matteVM_t * vm = matte_get_vm(m);

        matte_string_clear(infile);
        matte_string_clear(outfile);
        matte_string_concat_printf(infile, "test%d.mt", TESTID);
        matte_string_concat_printf(outfile, "test%d.out", TESTID);


        uint32_t lenBytes;
        uint8_t * src = dump_bytes(matte_string_get_c_str(infile), &lenBytes);
        if (!src) break;
        if (!lenBytes) {
            printf("Couldn't open source %s\n", matte_string_get_c_str(infile));
            exit(1);
        }
        printf("Running test %s...\n", matte_string_get_c_str(infile));
        fflush(stdout);
        uint32_t outByteLen;
        uint8_t * outBytes = matte_compiler_run(
            src,
            lenBytes,
            &outByteLen,
            onError,
            i+1
        );

        free(src);
        if (!outByteLen || !outBytes) {
            printf("Couldn't compile source %s\n", matte_string_get_c_str(infile));
            exit(1);
        }
        

        matteArray_t * arr = matte_bytecode_stubs_from_bytecode(outBytes, outByteLen);
        matte_vm_add_stubs(vm, arr);

        matteValue_t v = matte_vm_run_script(vm, i+1, arr);

        char * outstr = dump_string(matte_string_get_c_str(outfile));
        if (!outstr) {
            printf("Could not open output file %s.\n", matte_string_get_c_str(outfile));
            exit(1);
        }
        matteString_t * outputText = matte_string_create();
        matte_string_concat_printf(outputText, outstr);

        if (!matte_string_test_eq(outputText, matte_value_as_string(v))) {
            printf("Test failed!!\nExpected output   : %s\nReal output       : %s\n", outstr, matte_string_get_c_str(matte_value_as_string(v)));
            exit(1);
        }
        free(outstr);
        TESTID++;

        matte_destroy(m);
    }

    printf("Tests pass.");
    return 0;
}