#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../matte_string.h"

static void * dump_bytes(const char * filename, uint32_t * len) {
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


int main() {
    FILE * out = fopen("../CORE_ROM", "wb");
    if (!out) {
        printf("Could not open output ROM file.\n");
        exit(1);
    }
    fprintf(
        out,
        "// Core ROM files as strings. From makerom.c \n"
        "static char * MATTE_CORE_ROM[] = {\n"
    );

    char * files[] = {
        "class.mt",  "Matte.Class",
        "array.mt",  "Matte.Array",
        "string.mt", "Matte.String",
        "core.mt",   "Matte.Core",
        NULL
    };
    
    char ** iter = files;
    while(*iter) {
        uint32_t len;
        uint8_t * data = dump_bytes(iter[0], &len);
        matteString_t * str = matte_string_base64_from_bytes(data, len);

        fprintf(out, "\"%s\", \"%s\",\n", iter[1], matte_string_get_c_str(str));
        iter+=2;
        matte_string_destroy(str);
        free(data);
    }
    
    fprintf(out, "NULL};\n");
    fclose(out);
    return 0;
}