#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../matte_string.h"
#include "../matte_compiler.h"

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

static void onError(const matteString_t * s, uint32_t line, uint32_t ch, void * str) {
    printf(
        "Error compiling rom: %s\n(%s, line %d:%d)\n",
        matte_string_get_c_str(s),
        (char*)str,
        line, ch
    );
    exit(1);
}

int main() {
    FILE * out = fopen("../MATTE_ROM", "wb");
    if (!out) {
        printf("Could not open output ROM file.\n");
        exit(1);
    }
    fprintf(
        out,
        "// Core ROM files as strings. From makerom.c \n"
    );
    

    char * files[] = {
        "core/class.mt",      "Matte.Core.Class",
        "core/array.mt",      "Matte.Core.Array",
        "core/string.mt",     "Matte.Core.String",
        "core/json.mt",       "Matte.Core.JSON",
        "core/eventsystem.mt","Matte.Core.EventSystem",
        "core/core.mt",       "Matte.Core",
        #ifdef MATTE_USE_SYSTEM_EXTENSIONS
            "system/consoleio.mt",      "Matte.System.ConsoleIO",       
            "system/filesystem.mt",     "Matte.System.Filesystem",       
            "system/memorybuffer.mt",   "Matte.System.MemoryBuffer",       
            "system/socketio.mt",       "Matte.System.SocketIO",       
            "system/time.mt",           "Matte.System.Time",       
            "system/utility.mt",        "Matte.System.Utility",               
            "system/async.mt",        "Matte.System.Async",               
            "system/system.mt",         "Matte.System",       
        #endif

        NULL
    };

    matteString_t * names = matte_string_create_from_c_str(
        "const char * MATTE_ROM__names[MATTE_ROM__COUNT] = {\n"
    );
    matteString_t * sizes = matte_string_create_from_c_str(
        "const uint64_t MATTE_ROM__sizes[MATTE_ROM__COUNT] = {\n"
    );
    matteString_t * offsets = matte_string_create_from_c_str(
        "const uint64_t MATTE_ROM__offsets[MATTE_ROM__COUNT] = {\n"
    );
    matteString_t * datastr = matte_string_create_from_c_str(
        "const uint8_t MATTE_ROM__data[] = {\n"
    );


    
    char ** iter = files;
    int count = 0;
    uint64_t offset = 0;
    while(*iter) {
        uint32_t len;
        uint8_t * data = dump_bytes(iter[0], &len);
        uint32_t romCompiledLen = 0;
        uint8_t * romCompiledBytes = matte_compiler_run(
            data,
            len,
            &romCompiledLen,
            onError,
            iter[0]
        );

        matte_string_concat_printf(names, "\"%s\",\n", iter[1]);
        matte_string_concat_printf(sizes, "%d,\n", romCompiledLen);
        matte_string_concat_printf(offsets, "%d,\n", offset);
        offset += romCompiledLen;
    
        count++;
        matteString_t * strarr = matte_string_create_from_c_str("");
        uint32_t i;
        for(i = 0; i < romCompiledLen; ++i) {
            matte_string_concat_printf(strarr, "%3d,", romCompiledBytes[i]);
            if (i % 32 == 0) matte_string_concat_printf(strarr, "\n");
        }
        matte_string_concat(datastr, strarr);



        iter+=2;
        matte_string_destroy(strarr);
        free(data);
        free(romCompiledBytes);
    }
    matte_string_concat_printf(names, "};\n");
    matte_string_concat_printf(sizes, "};\n");
    matte_string_concat_printf(offsets, "};\n");
    matte_string_concat_printf(datastr, "};\n");

    
    
    fprintf(
        out,
        "#define MATTE_ROM__COUNT %d\n", 
        count
    );

    fprintf(
        out,
        "%s\n", 
        matte_string_get_c_str(names)
    );
    fprintf(
        out,
        "%s\n", 
        matte_string_get_c_str(sizes)
    );

    fprintf(
        out,
        "%s\n", 
        matte_string_get_c_str(offsets)
    );
    fprintf(
        out,
        "%s\n", 
        matte_string_get_c_str(datastr)
    );
    
    matte_string_destroy(names);
    matte_string_destroy(offsets);
    matte_string_destroy(sizes);
    matte_string_destroy(datastr);
    
    fclose(out);
    return 0;
}
