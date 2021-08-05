#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include "../src/matte_array.h"
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"


static void show_help() {
    printf("Command-Line tool for the Matte scripting language.\n");
    printf("Johnathan Corkery, 2021\n");
    printf("\n\n");
    printf("options:\n\n");

    printf("  [file(s)]\n");
    printf("    - When given a list of files, Matte will run the files sequentially\n");
    printf("      by first compiling them into bytecode and then running the\n");
    printf("      bytecode in one invocation. Output from each source is printed\n");
    printf("      to stdout.\n\n");

    printf("  exec [file(s)]\n");
    printf("    - When given a list of files, Matte will run the files sequentially\n");
    printf("      by assuming each file is a compiled bytecode blob.\n");
    printf("      Running a bytecode blob is done by running the root function\n");
    printf("      of the each fileID specified in the blob.\n\n");

    printf("  tokenize [file(s)]\n");
    printf("    - Assuming each file is a Matte source file, a token analysis is\n");
    printf("      run on each file and printed to stdout.\n\n");

    printf("  compile input-source.file fileID output.file\n");
    printf("    - Takes the given file and compiles it into a single bytecode\n");
    printf("      blob. The fileID of the given number is used\n\n");

    printf("  link [file(s)] output.file\n");
    printf("    - Takes the given bytecode blobs and combines them into a single\n");
    printf("      bytecode blob.\n\n");
    

    printf("  assemble [file(s)] output.file\n");
    printf("    - Takes the given Matte assembly files and compiles them into a \n");
    printf("      single bytecode blob.\n\n");

    printf("  disassemble [files] output.file\n");
    printf("    - Takes the given Matte bytecode blob files and, sequentially\n");
    printf("      writes the equivalent assembly for the bytecode blobs to\n");
    printf("      the output file. If only one file is specified, the output\n");
    printf("      is, instead, printed to standard out.\n\n");

}


static void onError(const matteString_t * s, uint32_t line, uint32_t ch) {
    printf("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    fflush(stdout);
}





int main(int argc, char ** args) {
    if (argc == 1) {
        show_help();
        exit(1);
    }

    const char * tool = args[1];
    

    if (!strcmp(tool, "assemble")) {
        if (argc < 3) {
            printf("Insufficient arguments for assemble tool\n");
            exit(1);
        }
        matte_assemble(
            args+2,
            argc-3,
            args[argc-1]
        );
        return 0;        
    } else if (!strcmp(tool, "disassemble")) {
        if (argc < 3) {
            printf("Insufficient arguments for disassemble tool\n");
            exit(1);
        }
        matte_disassemble(
            args+2,
            argc-3,
            args[argc-1]
        );
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
            matte_compiler_tokenize(
                dump,
                fsize,
                onError,
                1
            );
            free(dump);
        }
    } else if (!strcmp(tool, "exec")) {
        if (argc < 3) {
            printf("Insufficient arguments for compile tool\n");
            exit(1);
        }

        matte_t * m = matte_create();
        matteVM_t * vm = matte_get_vm(m);

        matteArray_t * fileID = matte_array_create(sizeof(uint32_t));
        int n = argc-2;
        uint32_t i;
        for(i = 0; i < n; ++i) {
            uint32_t len;
            void * data = dump_bytes(args[2+i], &len);
            if (!data) {
                printf("Couldn't open file %s\n", args[2+i]);
                exit(1);
            }
            matteArray_t * arr = matte_bytecode_stubs_from_bytecode(data, len);
            if (arr) {
                uint32_t j;
                uint32_t jlen = matte_array_get_size(arr);
                for(j = 0; j < jlen; ++j) {
                    matteBytecodeStub_t * stub = matte_array_at(arr, matteBytecodeStub_t *, j);
                    uint32_t k;
                    uint32_t klen = matte_array_get_size(fileID);
                    int addID = 1;
                    for(k = 0; k < klen; ++k) {
                        if (matte_array_at(fileID, uint32_t, k) == matte_bytecode_stub_get_file_id(stub)) {
                            addID = 0;
                        }
                    }
                    if (addID) {
                        uint32_t fileid = matte_bytecode_stub_get_file_id(stub);
                        matte_array_push(fileID, fileid);
                    }
                }
                matte_vm_add_stubs(vm, arr);
            } else {
                printf("Failed to compile bytecode from input file %s\n", args[i+2]);
                exit(1); 
            }
        }
        matteArray_t * arr = matte_array_create(sizeof(matteValue_t));        

        uint32_t len = matte_array_get_size(fileID);
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_vm_run_script(vm, matte_array_at(fileID, uint32_t, i), arr);
            printf("> %s\n", matte_string_get_c_str(matte_value_as_string(v)));
        }
        return 0;
        
    } else if (!strcmp(tool, "compile")) {
        if (argc != 5) {
            printf("Insufficient arguments for compile tool\n");
            exit(1);
        }
        uint32_t sourceLen;
        uint8_t * source = dump_bytes(args[2], &sourceLen);
        if (!source) {
            printf("Couldn't open input file %s\n", args[2]);
            exit(1);
        }

        int fileID = atoi(args[3]);        


        uint32_t outByteLen;
        uint8_t * outBytes = matte_compiler_run(
            source,
            sourceLen,
            &outByteLen,
            onError,
            fileID
        );

        if (!outBytes) {
            printf("Unable to compile input file.\n");
            exit(1);
        }

        FILE * out = fopen(args[4], "wb");
        if (!out) {
            printf("Unable to compile (output file %s inaccessible)\n", args[4]);
            exit(1);
        }
        fwrite(outBytes, 1, outByteLen, out);
        fclose(out);
    } else if (!strcmp(tool, "link")) {
        if (argc < 4) {
            printf("Insufficient arguments for link tool\n");
            exit(1);
        }

        FILE * out = fopen(args[argc-1], "wb");
        if (!out) {
            printf("Couldn't open output file %s\n", args[argc-1]);
        }

        uint32_t i;
        uint32_t len = argc-3;
        for(i = 0; i < len; ++i) {
            uint32_t blen;
            uint8_t * bytes = dump_bytes(args[i+2], &blen);
            if (!blen || !bytes) {
                printf("Couldn't open / read input bytecode file %s\n", args[i+2]);
                exit(1);
            }

            fwrite(bytes, 1, blen, out);
        }
        fclose(out);
    } else { // just filenames 
        uint32_t i;
        uint32_t len = argc-1;
        
        matte_t * m = matte_create();
        matteVM_t * vm = matte_get_vm(m);

        // first compile all and add them 
        for(i = 0; i < len; ++i) {
            uint32_t lenBytes;
            uint8_t * src = dump_bytes(args[i+1], &lenBytes);
            if (!src || !lenBytes) {
                printf("Couldn't open source %s\n", args[i+1]);
                exit(1);
            }

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
                printf("Couldn't compile source %s\n", args[i+1]);
                exit(1);
            }
            

            matteArray_t * arr = matte_bytecode_stubs_from_bytecode(outBytes, outByteLen);
            matte_vm_add_stubs(vm, arr);
        }    

        matteArray_t * arr = matte_array_create(sizeof(matteValue_t));        
        for(i = 0; i < len; ++i) {
            matteValue_t v = matte_vm_run_script(vm, i+1, arr);
            printf("> %s\n", matte_string_get_c_str(matte_value_as_string(v)));
        }
        

    }
    return 0;
}
