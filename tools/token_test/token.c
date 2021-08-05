#include "../../src/matte.h"
#include "../../src/matte_vm.h"
#include "../../src/matte_bytecode_stub.h"
#include "../../src/matte_array.h"
#include "../../src/matte_string.h"
#include "../../src/matte_compiler.h"
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

static void onError(const matteString_t * s, uint32_t line, uint32_t ch) {
    printf("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    fflush(stdout);
}


int main(int argc, char ** args) {
    if (argc != 2) {
        printf("Syntax: %s [script path]\n", args[0]);
        return 0;
    }

    uint32_t byteLen;
    uint8_t * d = dump_bytes(args[1], &byteLen);
    uint8_t * output = matte_compiler_run(d, byteLen, &byteLen, onError, 1);

    if (output && byteLen) {
        FILE * f = fopen("a.out", "wb");
        fwrite(output, 1, byteLen, f);
        fclose(f);
    } else {
        printf("Compilation failed. No file written.\n");
    }
    return 0;
}
