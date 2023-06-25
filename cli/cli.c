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
#include "settings.h"



// whether the repl should keep going
static int KEEP_GOING = 1;

// whether debugging was enabled.
static int DEBUG = 0;

// Settings instance 
static matteSettings_t * settings = NULL;

// When importing files, this is used as a prefix.
// This is used when testing packages.
static matteString_t * import_namespace = NULL;



// when a user calls "exit()" this signals the end of the repl.
static matteValue_t repl_exit(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    KEEP_GOING = 0;
    matteStore_t * store = matte_vm_get_store(vm);
    
    return matte_store_new_value(store);
};



static int load_package_recursive(matte_t * m, const char * name) {
    // already loaded
    if (matte_value_type(matte_get_package_info(m, name)) != MATTE_VALUE_TYPE_EMPTY)
        return 1;
        
    const char * packagesPath = matte_settings_get_package_path(settings);
    if (packagesPath != NULL) {
        matteString_t * packagePath = matte_string_create_from_c_str("%s%s", packagesPath, name);
        uint32_t byteLen;
        uint8_t * bytes = (uint8_t*)dump_bytes(matte_string_get_c_str(packagePath), &byteLen, 0);
        matte_string_destroy(packagePath);
        if (bytes == NULL || byteLen == 0) {
            return 0;
        }


        matte_load_package(m, bytes, byteLen);
        

        
        // load dependencies
        matteArray_t * dependencies = matte_get_package_dependencies(m, name);
        uint32_t i;
        uint32_t len = matte_array_get_size(dependencies);
        for(i = 0; i < len; ++i) {
            matteString_t * str = matte_array_at(dependencies, matteString_t *, i);
            load_package_recursive(m, matte_string_get_c_str(str));
            matte_string_destroy(str);
        }
        matte_array_destroy(dependencies);
        
        
        // check to make sure we got all dependencies loaded correctly.
        return matte_check_package(m, name);
    }
    return 0;

}

static uint32_t cli_importer(
    matte_t * m,
    const char * name,
    void * userdata
) {
    
    
    // first check if this name aliases something within 
    // the package path.
    if (!settings) {
        settings = (void*)0x1;
        settings = matte_settings_load(m);
    }
    if (settings && settings != (void*)0x1) {
        if (load_package_recursive(m, name)) {
            return matte_vm_get_file_id_by_name(matte_get_vm(m), MATTE_VM_STR_CAST(matte_get_vm(m), name)); 
        }
    }
    
    uint32_t byteLen;
    uint8_t * bytes;
    if (import_namespace && strstr(name, matte_string_get_c_str(import_namespace)) == name) {
        // remove namespace
        matteString_t * str = matte_string_create_from_c_str("%s", name);
        matte_string_remove_n_chars(str, 0, matte_string_get_length(import_namespace));
        
        bytes = (uint8_t*)dump_bytes(matte_string_get_c_str(str), &byteLen, 0);
        matte_string_destroy(str);
    } else {
        // dump bytes
        bytes = (uint8_t*)dump_bytes(name, &byteLen, 0);
    }
    if (byteLen == 0 || bytes == NULL) {
        return 0;
    }
        
    

    
    uint32_t fileid = matte_vm_get_new_file_id(matte_get_vm(m), MATTE_VM_STR_CAST(matte_get_vm(m), name));   
    // determine if bytecode or raw source OR package.
    // handle bytecodecase
    if (byteLen >= 6 &&
        bytes[0] == 'M'  &&
        bytes[1] == 'A'  &&
        bytes[2] == 'T'  &&
        bytes[3] == 0x01 &&
        bytes[4] == 0x06 &&
        bytes[5] == 'B') {
        
        
        matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
            matte_vm_get_store(matte_get_vm(m)),
            fileid,
            bytes,
            byteLen
        );
        if (stubs) {
            matte_vm_add_stubs(matte_get_vm(m), stubs);
        } else {
            fileid = 0; // failed.
            //matte_print(m, "Failed to assemble bytecode %s.", name); 
        }        
    // raw source
    } else {
        char * source = matte_allocate(byteLen+1);
        memcpy(source, bytes, byteLen);
        uint32_t bytecodeLen;
        uint8_t * bytecode = matte_compile_source(
            m,
            &bytecodeLen,
            source
        );
        if (DEBUG)
            matte_debugging_register_source(m, fileid, source);
            
        matte_deallocate(source);
        
        if (!bytes || ! bytecodeLen) {
            matteString_t * str = matte_string_create_from_c_str("Could not import '%s'.", name);
            matte_vm_raise_error_string(matte_get_vm(m), str);
            matte_string_destroy(str);
            fileid = 0;
        } else {        
        
            matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
                matte_vm_get_store(matte_get_vm(m)),
                fileid,
                bytecode,
                bytecodeLen
            );
            if (stubs) {
            matte_vm_add_stubs(matte_get_vm(m), stubs);
            matte_array_destroy(stubs);
            } else {
                fileid = 0; // failed.
                //matte_print(m, "Failed to assemble bytecode %s.", name); 
            }           
        }
        matte_deallocate(bytecode);
    }

    matte_deallocate(bytes);
    return fileid;
}


static int repl() {
    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL);    
    matte_set_importer(m, cli_importer, NULL);

    matteVM_t * vm = matte_get_vm(m);
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t state = matte_store_new_value(store);

    matte_value_into_new_object_ref(store, &state);
    matte_value_object_push_lock(store, state);
    
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
    matte_destroy(m); 
    
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




matteValue_t packager_compile(
    matteVM_t * vm, 
    matteValue_t fn, 
    const matteValue_t * args, 
    void * userData
) {
    matte_t * m = (matte_t *) userData;
    matteStore_t * store = matte_vm_get_store(vm);
    const matteString_t * input  = matte_value_string_get_string_unsafe(store, args[0]);
    const matteString_t * output = matte_value_string_get_string_unsafe(store, args[1]);
    
    
    uint32_t srcLen = 0;
    uint8_t * source = dump_bytes(matte_string_get_c_str(input), &srcLen, 0);
    if (srcLen == 0 || !source) {
        printf("Could not open source %s for compilation.\n", matte_string_get_c_str(input));    
        exit(1);
    }
    char * srcStr = matte_allocate(srcLen+1);
    memcpy(srcStr, source, srcLen);
    
    
    uint32_t byteLen = 0;
    uint8_t * bytes = matte_compile_source(m, &byteLen, srcStr);
    if (!bytes || !byteLen)
        exit(1);
        
        
    FILE * out = fopen(matte_string_get_c_str(output), "wb");
    if (!out)  {
        printf("Could not access output file %s\n", matte_string_get_c_str(output));
        exit(1);
    }
    if (fwrite(bytes, byteLen, 1, out) != byteLen) {    
        printf("Could not access output file %s\n", matte_string_get_c_str(output));
        exit(1);
    }
    
    fclose(out);
    
    return matte_store_new_value(matte_vm_get_store(matte_get_vm(m)));
    
}
matteValue_t packager_run_debug(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    matte_t * m = (matte_t *)userData;
    matteStore_t * store = matte_vm_get_store(vm);
    matte_debugging_enable(m);
    DEBUG = 1;
    return matte_vm_import(
        vm,
        matte_value_string_get_string_unsafe(store, args[0]),
        matte_store_new_value(store)
    );
    
}
matteValue_t packager_set_import(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    matteStore_t * store = matte_vm_get_store(vm);    
    if (import_namespace)
        matte_string_destroy(import_namespace);
    import_namespace = matte_string_create_from_c_str(
        "%s.",
        matte_string_get_c_str(matte_value_string_get_string_unsafe(store, args[0]))
    );
}
matteValue_t packager_get_system_path(matteVM_t * vm, matteValue_t fn, const matteValue_t * args, void * userData) {
    matte_t * m = (matte_t*)userData;
    if (!settings) settings = matte_settings_load(m);

    matteString_t * outstr = matte_string_create_from_c_str(matte_settings_get_package_path(settings));
    matteStore_t * store = matte_vm_get_store(matte_get_vm(m));
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_string(store, &out, outstr);
    matte_string_destroy(outstr);
    return out;
}

#include "package/package.mt.h"

static int packager(matte_t * m, int argc, char ** args) {
    matte_add_external_function(m, "package_matte_compile",         packager_compile, m, "source", "output", NULL);
    matte_add_external_function(m, "package_matte_run_debug",       packager_run_debug, m, "source", NULL);
    matte_add_external_function(m, "package_matte_set_import",      packager_set_import, m, "namespace", NULL);
    matte_add_external_function(m, "package_matte_get_system_path", packager_get_system_path, m, NULL);    


    // convert args into


    /*matte_run_bytecode(m,
        PACKAGE_MT_BYTES,
        PACKAGE_MT_SIZE,*/
        
    return 1;
}



int main(int argc, char ** args) {
    if (argc == 1) {
        return repl();
    }

    const char * tool = args[1];


    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL); // standard IO is fine
    matte_set_importer(m, cli_importer, NULL); // standard file import is fine
    
    if (!strcmp(tool, "package")) {
        return packager(m, argc, args);
    
    } else if (!strcmp(tool, "--help") ||
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
        DEBUG = 1;
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
            uint8_t * dump = dump_bytes(args[2+i], &fsize, 1);
            if (!dump) {
                printf("Could not open input file %s\n", args[2+i]);
                exit(1);
            }
            matteString_t * str = matte_compiler_tokenize(
                matte_get_syntax_graph(m),
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
        uint8_t * source = dump_bytes(args[2], &sourceLen, 1);
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
        matte_store_recycle(matte_vm_get_store(vm), v);
        matte_destroy(m);
    }
    return 0;
}
