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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"



// whether the repl should keep going
static int KEEP_GOING = 1;

// when a user calls "exit()" this signals the end of the repl.
static matteValue_t repl_exit(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    KEEP_GOING = 0;
    matteHeap_t * heap = matte_vm_get_heap(vm);
    
    
    
    return matte_heap_new_value(heap);
};


static int repl() {
    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL);    

    matteVM_t * vm = matte_get_vm(m);
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t state = matte_heap_new_value(heap);

    matte_value_into_new_object_ref(heap, &state);
    matte_value_object_push_lock(heap, state);
    
    matte_add_external_function(m, "repl_exit", repl_exit, NULL, NULL);
    matteValue_t exitFunc = matte_run_source(m, "return getExternalFunction(name:'repl_exit');");
    
    printf("Matte REPL.\n");
    printf("Johnathan Corkery, 2023\n");
    printf("Expression mode: 'store' variable exists to store values. exit by calling exit().\n\n");
    
    while(KEEP_GOING) {
        #define REPL_LINE_LIMIT 1024
        printf(">> ");
        
        char line[REPL_LINE_LIMIT+1];
        fgets(line, REPL_LINE_LIMIT, stdin); 
        
        matteString_t * source = matte_string_create_from_c_str("return ::(exit, store){return (%s);};", line);
        matteValue_t nextFunc = matte_run_source(m, matte_string_get_c_str(source));
        matte_string_destroy(source);

        if (matte_value_type(nextFunc) != MATTE_VALUE_TYPE_EMPTY) {
            matteValue_t result = matte_call(
                m,
                nextFunc,
                "exit", exitFunc,
                "store", state,
                NULL
            );
            printf("%s\n\n", matte_introspect_value(m, result));        
        }
    }    
    
    return 0;

}



static void show_help() {
    printf("Command-Line tool for the Matte scripting language.\n");
    printf("Johnathan Corkery, 2023\n\n");
    printf("Running this program with no arguments enters a REPL mode.\n\n");
    printf("options:\n\n");

    printf("  --help\n");
    printf("  help\n");
    printf("  -h\n");
    printf("  -v\n");
    printf("    - Displays this help text.\n\n");

    printf("  [file] [args...]\n");
    printf("    - When given a file, Matte will run the file\n");
    printf("      by first importing it and then running the\n");
    printf("      bytecode in one invocation. Output from each source is printed\n");
    printf("      to stdout.\n\n");

    printf("  debug [file] [args...]\n");
    printf("    - When given a file, Matte will run the file in debug mode\n");
    printf("      In this mode, matte will accept gdb-style commands before\n");
    printf("      and during execution. When in this mode, type 'help' and\n");
    printf("      enter for more information.\n");

    printf("  tokenize [files]\n");
    printf("    - Assuming each file is a Matte source file, a token analysis is\n");
    printf("      run on each file and printed to stdout.\n\n");

    printf("  compile input-source.file output.file\n");
    printf("    - Takes the given file and compiles it into a single bytecode\n");
    printf("      blob. The fileID of the given number is used\n\n");


}


static void tokenize_error(const matteString_t * str, uint32_t line, uint32_t ch, void * data) {
    printf("Tokenize error: %s (line %d:%d)", matte_string_get_c_str(str), line, ch);
}



int main(int argc, char ** args) {
    if (argc == 1) {
        return repl();
    }

    const char * tool = args[1];


    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL); // standard IO is fine
    
    if (!strcmp(tool, "--help") ||
        !strcmp(tool, "help") ||
        !strcmp(tool, "-h") ||
        !strcmp(tool, "-v")) {
        show_help();
        return 0;
    } else if (!strcmp(tool, "debug")) {
        if (argc < 3) {
            printf("Syntax: matte debug [file] [parameters]\n");
            exit(1);
        }        
        matte_debugging_enable(m);
        matteVM_t * vm = matte_get_vm(m);
        matte_vm_import(
            vm,
            MATTE_VM_STR_CAST(vm, args[2]),
            parse_parameters(vm, args+3, argc-3)
        );
        return 0;        
    } else if (!strcmp(tool, "tokenize")) {        
        if (argc < 3) {
            printf("Insufficient arguments for tokenize tool\n");
            exit(1);
        }
        uint32_t i;
        uint32_t len = argc-2;
        for(i = 0; i < len; ++i) {
            uint32_t fsize;
            uint8_t * dump = dump_bytes(args[2+i], &fsize);
            if (!dump) {
                printf("Could not open input file %s\n", args[2+i]);
                exit(1);
            }
            matteString_t * str = matte_compiler_tokenize(
                dump,
                fsize,
                tokenize_error,
                NULL
            );
            free(dump);

            if (str) {
                printf("%s\n", matte_string_get_c_str(str));
                matte_string_destroy(str);
            }
        }
    } else if (!strcmp(tool, "compile")) {
        if (argc != 4) {
            printf("Insufficient arguments for compile tool\n");
            exit(1);
        }
        uint32_t sourceLen;
        uint8_t * source = dump_bytes(args[2], &sourceLen);
        if (!source) {
            printf("Couldn't open input file %s\n", args[2]);
            exit(1);
        }

        char * str = malloc(sourceLen+1);
        memcpy(str, source, sourceLen);
        str[sourceLen] = 0;

        uint32_t outByteLen;
        uint8_t * outBytes = matte_compile_source(
            m,
            &outByteLen,
            str
        );
        
        if (!outBytes) {
            printf("Unable to compile input file.\n");
            exit(1);
        }

        FILE * out = fopen(args[3], "wb");
        if (!out) {
            printf("Unable to compile (output file %s inaccessible)\n", args[3]);
            exit(1);
        }
        fwrite(outBytes, 1, outByteLen, out);
        fclose(out);
    } else { // just filenames 
        uint32_t i = 0;
        uint32_t len = argc-1;
        matteVM_t * vm = matte_get_vm(m);
        
        matteValue_t params = parse_parameters(vm, args+2, argc-2);
        matteValue_t v = matte_vm_import(vm, MATTE_VM_STR_CAST(vm, args[i+1]), params);
        matte_heap_recycle(matte_vm_get_heap(vm), v);
        matte_destroy(m);
    }
    return 0;
}
