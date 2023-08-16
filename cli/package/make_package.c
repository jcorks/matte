#include "../shared.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char ** argv) {
    uint32_t byteLen;
    uint8_t * bytes = dump_bytes("package.mt", &byteLen, 1);
    if (!byteLen || !bytes) {
        printf("Couldnt open package.mt\n");
        exit(1);
    }
    

    char * str = malloc(byteLen+1);
    memcpy(str, bytes, byteLen);
    str[byteLen] = 0;
    
    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL);
    uint32_t bytecodeSize;
    uint8_t * bytecode = matte_compile_source(
        m,
        &bytecodeSize,
        str
    );
    free(str);
    
    
    
    FILE * output = fopen("package.mt.h", "wb");
    fprintf(output, "static uint32_t PACKAGE_MT_SIZE = %d;\nstatic uint8_t PACKAGE_MT_BYTES[] = {\n", (int)bytecodeSize);
    uint32_t i;
    for(i = 0; i < bytecodeSize; ++i) {
        fprintf(output, "%d,", bytecode[i]);
        if (i%128==0) fprintf(output, "\n");
    }
    fprintf(output, "};\n");
    fclose(output);
    return 0;
}
