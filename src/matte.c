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
#include "matte.h"
#include "matte_vm.h"
#include "matte_table.h"
#include "matte_string.h"
#include "matte_array.h"
#include "matte_compiler.h"
#include "matte_compiler__syntax_graph.h"
#include "matte_bytecode_stub.h"
#include "matte_store.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    uint32_t fileid;
    uint32_t line;
} breakpoint;


#define MATTE_COMMAND_LIMIT 256




static void * (*matte_allocate_fn)  (uint64_t) = NULL;
static void   (*matte_deallocate_fn)(void *)   = NULL;

typedef struct {
    // Name of the package
    matteString_t * name;
    // The major version of the package
    int versionMajor;
    // The minor version of the package
    int versionMinor;
    // The parsed package json
    matteValue_t packageJson;
} mattePackageInfo_t;



struct matte_t {
    matteVM_t * vm;
    void * userdata;

    // The assigned input function. Always populated
    char * (*input) (matte_t *);
    
    // The assigned output function. Always populated
    void   (*output)(matte_t *, const char *);
    
    // Function to clear output. Always populated
    void   (*clear)(matte_t *);
    
    // importer function for when set_importer is called
    uint32_t (*importer)(matte_t *, const char *, const char *, void *);
    
    // importer user data
    void * importerData;
    
    // cached syntax graph.
    matteSyntaxGraph_t * graph;
    
    // whether the debug contexT is enabled.
    int isDebug;
    
    // Tracks how many matte_source_run and related functions have been called.
    // These are used for the dummy fileIDs in the case that there is no real filename.
    int runSourceCount;
    
    // whether the next debug event is to be paused against
    int pauseNext;
    
    // The required frame to step to. Used for step vs next
    int stepreqframe;
    
    // Database of fileID to array of line strings.
    // Used for debugging print areas.
    matteTable_t * lines; // fileid -> array of lines (matteString);
    
    
    // result of last introspect.
    matteString_t * introspectResult;
    
    // List of breakpoints. currently not used.
    matteArray_t * breakpoints;
    
    // Last compiler run error if applicable
    matteString_t * lastCompilerError;
    
    // current stackframe in debugger
    uint32_t stackframe;
    
    // Last prompt input
    char * lastInput;
    
    // Current prompt input
    char * currentInput;
    
    // package name string -> mattePackageInfo_t *
    matteTable_t * packages;
};


// Implementation of strdup since it is not part of C99 standard
static char * matte_strdup(const char * str) {
    int len = strlen(str);
    char * cpy = (char*)matte_allocate(len+1);
    memcpy(cpy, str, len+1);
    return cpy;   
}

// Convenience function for printf style print to 
// output function for matte instance
#define MATTE_PRINT_LIMIT 4096
static void matte_print(matte_t * m, const char * fmt, ...) {
    int size;
    {
        va_list args;
        va_start(args, fmt);
        size = vsnprintf(
            NULL,
            0,
            fmt,
            args
        );
    }
    if (size <= 0) return;
    va_list args;
    va_start(args, fmt);
    char * buffer = (char*)matte_allocate(size+2);
    vsnprintf(
        buffer,
        size+1,
        fmt,
        args
    );    
    m->output(m, buffer);
    matte_deallocate(buffer);
}


// Uses matte_print to print a localized 
// version of the lines of code involved with the debug area.
#define PRINT_AREA_LINES 20
static void debug_print_area(matte_t * m) {
    matteVM_t * vm = m->vm;
    matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, m->stackframe);
    uint32_t fileid = matte_bytecode_stub_get_file_id(frame.stub);
    uint32_t numinst;
    const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);
    uint32_t line = 0;
    if (frame.pc-1 >= 0 && frame.pc-1 < numinst)
        line = inst[frame.pc-1].info.lineOffset + matte_bytecode_stub_get_starting_line(frame.stub);

    m->clear(m);
    matte_print(m, "<file %s, line %d>", 
        matte_vm_get_script_name_by_id(vm, fileid) ? 
            matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, fileid)) 
        : 
            "???" , 
        line
    );
    int i = line;

    matteArray_t * localLines = (matteArray_t*)matte_table_find_by_uint(m->lines, fileid);
    if (localLines) {
        for(i = ((int)line) - PRINT_AREA_LINES/2; i < ((int)line) + PRINT_AREA_LINES/2 + 1; ++i) {
            if (i < 0 || i >= matte_array_get_size(localLines)) {
                matte_print(m, "  ---- | ");
            } else {
                if (i == line-1) {
                    matte_print(m, "->%4d | %s", i+1, matte_string_get_c_str(matte_array_at(localLines, matteString_t *, i)));
                } else {
                    matte_print(m, "  %4d | %s", i+1, matte_string_get_c_str(matte_array_at(localLines, matteString_t *, i)));
                }
            }
        }
    }
    matte_print(m, "");
}


// Default handler for compile errors.
static void default_compile_error(const matteString_t * str, uint32_t line, uint32_t ch, void * data) {
    matte_t * m = (matte_t*)data;
    if (m->lastCompilerError)
        matte_string_destroy(m->lastCompilerError);
    m->lastCompilerError = matte_string_create_from_c_str(
        "%s (line %d:%d)", matte_string_get_c_str(str), line, ch
    );
}

static char * prompt(matte_t * m) {
    matte_print(m, "[Matte Debugging]:");
    
    if (m->currentInput)
        matte_deallocate(m->currentInput);
        
    while(!(m->currentInput = m->input(m)));
    return m->currentInput;
}

static void exec_command_print_error(
    matteVM_t * vm, 
    matteVMDebugEvent_t event, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * data
) {
    matte_t * m = (matte_t*)data;
    if(event == MATTE_VM_DEBUG_EVENT__ERROR_RAISED)
        matte_print(m, "Error while evaluating print command: %s", matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_store(vm), matte_value_as_string(matte_vm_get_store(vm), val))));
}

static int exec_command(matte_t * m) {
    matteVM_t * vm = m->vm;
    prompt(m); // populates currentCommand
    if (m->currentInput[0] == '\n' || m->currentInput[0] == 0) {
        // repeat last command;
        matte_deallocate(m->currentInput);        
        m->currentInput = matte_strdup(m->lastInput);
    } else {
        matte_deallocate(m->lastInput);
        m->lastInput = matte_strdup(m->currentInput);
    }

    // command is the first word
    char * iter = matte_strdup(m->currentInput);
    char * res = m->currentInput; // "res"t of command
    char * command = iter;
    while(*res && !isspace(*res)) {
        *iter = *res;
        iter++; 
        res++;
    }
    *iter = 0;


    // we follow GDB rules, except we dont use breakpoints.
    // Breakpoints are left in the code specifically 
    if (!strcmp(command, "step") ||
               !strcmp(command, "s")) {
        m->pauseNext = 1;
        m->stepreqframe = -1;
        return 0;
        
    } else if (!strcmp(command, "next") ||
               !strcmp(command, "n")) {
        m->pauseNext = 1;
        m->stepreqframe = matte_vm_get_stackframe_size(vm);
        return 0;
    } else if (!strcmp(command, "continue")||
               !strcmp(command, "c")) {
        return 0;
    } else if (!strcmp(command, "backtrace") ||
               !strcmp(command, "bt")) {
        uint32_t i;
        uint32_t len = matte_vm_get_stackframe_size(vm);
        for(i = 0; i < len; ++i) {
            matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, i);
            uint32_t numinst;
            const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);

            uint32_t fileid = matte_bytecode_stub_get_file_id(frame.stub);
            const matteString_t * fileName = matte_vm_get_script_name_by_id(vm, fileid);
            uint32_t lineNumber = inst[frame.pc].info.lineOffset + matte_bytecode_stub_get_starting_line(frame.stub); 
            if (i == m->stackframe) {
                matte_print(m, " -> @%d: <%s>, line %d", i, fileName ? matte_string_get_c_str(fileName) : "???", (int)lineNumber);
            } else {
                matte_print(m, "    @%d: <%s>, line %d", i, fileName ? matte_string_get_c_str(fileName) : "???", (int)lineNumber);
            }
        }

        
    } else if (!strcmp(command, "up") ||    
               !strcmp(command, "u")) {
        if (m->stackframe < matte_vm_get_stackframe_size(vm) - 1) {
            m->stackframe++;
            debug_print_area(m);
        } else {
            matte_print(m, "Already at top of callstack.");
        }
    } else if (!strcmp(command, "down") ||    
               !strcmp(command, "d")) {
        if (m->stackframe > 0) {
            m->stackframe--;
            debug_print_area(m);
        } else {
            matte_print(m, "Already at bottom of callstack.");
        }
    } else if (!strcmp(command, "print") ||    
               !strcmp(command, "p")) {
        matteString_t * src = matte_string_create_from_c_str("return (%s);", res);        
        matteValue_t val = matte_vm_run_scoped_debug_source(
            vm,
            src,
            m->stackframe,
            exec_command_print_error,
            m
        );
        matte_print(m, matte_introspect_value(m, val));

    } else {
        matte_print(m, "Unknown command.");        
    }
    return 1;
}

static void debug_split_lines(matte_t * m, uint32_t fileid, const uint8_t * data, uint32_t size) {
    matteArray_t * localLines = matte_array_create(sizeof(matteString_t *));
    uint32_t i;
    matteString_t * line = matte_string_create();
    for(i = 0; i < size; ++i) {
        if (data[i] == '\n') {
            matte_array_push(localLines, line);
            line = matte_string_create();
        } else {
            if (data[i] != '\r')
                matte_string_append_char(line, data[i]);
        }
    }
    matte_array_push(localLines, line);
    matte_table_insert_by_uint(m->lines, fileid, localLines);
}





static void default_debug_callback(
    matteVM_t * vm, 
    matteVMDebugEvent_t event, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * data
) {
    matte_t * m = (matte_t*)data;
    if (event == MATTE_VM_DEBUG_EVENT__ERROR_RAISED) {
        const matteString_t * str = matte_value_string_get_string_unsafe(matte_vm_get_store(vm), matte_value_as_string(matte_vm_get_store(vm), val));
        matte_print(m, "ERROR RAISED FROM VIRTUAL MACHINE: %s", str ? matte_string_get_c_str(str) : "<no string info given>");
    }
}



static void debug_on_event(
    matteVM_t * vm, 
    matteVMDebugEvent_t event, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * data
) {
    matte_t * m = (matte_t*)data;
    if (event == MATTE_VM_DEBUG_EVENT__ERROR_RAISED) {
        // error happened before stackframe could start, so its not super useful to start at 0
        if (matte_vm_get_stackframe(vm, m->stackframe).pc == 0)
            m->stackframe++;

        debug_print_area(m);
        const matteString_t * str = matte_value_string_get_string_unsafe(matte_vm_get_store(vm), matte_value_as_string(matte_vm_get_store(vm), val));
        matte_print(m, "ERROR RAISED FROM VIRTUAL MACHINE: %s", str ? matte_string_get_c_str(str) : "<no string info given>");
        while(exec_command(m));
        return;    
    }
    
    if (event == MATTE_VM_DEBUG_EVENT__BREAKPOINT) {
        m->stackframe = 0;
        debug_print_area(m);
        matte_print(m, "Source breakpoint reached.");
        while(exec_command(m));
        return;
    }
    
     
    if (m->pauseNext &&
        (m->stepreqframe == -1 ||
         matte_vm_get_stackframe_size(m->vm) <= m->stepreqframe)
    ) {
        m->stackframe = 0;
        m->pauseNext = 0;
        debug_print_area(m);
        while(exec_command(m));
        return;
    }



    /*
    uint32_t i;
    for(i = 0; i < matte_array_get_size(m->breakpoints); ++i) {
        breakpoint p = matte_array_at(m->breakpoints, breakpoint, i);
        if (p.fileid == file &&
            p.line == lineNumber) {
            m->stackframe = 0;
            debug_print_area(m);
            matte_print(m, "Breakpoint %d reached.\n", i);
            while(exec_command(m));
        }
    }
    */
    
    
}


static char * default_input(matte_t * m) {
    printf(" > ");
    char * line = (char*)matte_allocate(256+1);;
    fgets(line, 256, stdin);
    return line;
}

static void default_output(matte_t * m, const char * str) {
    printf("%s\n", str);
    fflush(stdout);
}

static void default_clear(matte_t * m) {
    // ansi clear screen
    printf("\x1b[2J");
}


static void default_unhandled_error(
    matteVM_t * vm, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * d
) {
    matte_t * m = (matte_t*)d;
    if (matte_value_type(val) == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t s = matte_value_object_access_string(matte_vm_get_store(vm), val, MATTE_VM_STR_CAST(vm, "summary"));
        if (matte_value_type(s) && matte_value_type(s) == MATTE_VALUE_TYPE_STRING) {
            matte_print(m, 
                "Unhandled error: %s", 
                matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_store(vm), s))
            );
            return;
        }
    }
    
    if (matte_vm_get_script_name_by_id(vm, file)) {
        matte_print(m, 
            "Unhandled error (%s, line %d)", 
            matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
            lineNumber
        );
        matte_print(m, "%s", matte_introspect_value(m, val));
    };
}



static void default_print_callback(matteVM_t * vm, const matteString_t * str, void * data) {
    matte_t * m = (matte_t*)data;
    m->output(
        m,
        matte_string_get_c_str(str)
    );
}


static uint32_t matte_importer(
    // The VM instance
    matteVM_t * vm,
    // The name of the module being requested
    const matteString_t * name,
    
    const matteString_t * alias,
    
    void * usrdata
) {
    matte_t * m = (matte_t*)usrdata;
    return m->importer(m, matte_string_get_c_str(name), alias ? matte_string_get_c_str(alias) : NULL, m->importerData);
}

static uint32_t default_importer(
    matte_t * m,
    const char * name,
    const char * alias,
    void * userdata
) {

    // first check if we're dealing with a package.

    
    // dump bytes
    FILE * f = fopen(name, "rb");
    if (!f) {
        matteString_t * str = matte_string_create_from_c_str("Could not import file '%s'. File could not be accessed or found.", name);
        matte_vm_raise_error_string(m->vm, str);
        matte_string_destroy(str);
        return 0;
    }
    char chunk[2048];
    int chunkSize;
    uint32_t bytelen = 0;    
    while(chunkSize = (fread(chunk, 1, 2048, f))) bytelen += chunkSize;
    fseek(f, 0, SEEK_SET);


    uint8_t * bytes = (uint8_t*)matte_allocate(bytelen);
    uint32_t iter = 0;
    while(chunkSize = (fread(chunk, 1, 2048, f))) {
        memcpy(bytes+iter, chunk, chunkSize);
        iter += chunkSize;
    }
    fclose(f);

    uint32_t fileID = matte_add_module(
        m,
        alias ? alias : name,
        bytes,
        bytelen
    );

    matte_deallocate(bytes);
    return fileID;
}


matte_t * matte_create() {
    matte_t * m = (matte_t*)matte_allocate(1*sizeof(matte_t));
    m->output = default_output;
    m->input = default_input;
    m->graph = matte_syntax_graph_create();
    m->vm = matte_vm_create(m);
    m->packages = matte_table_create_hash_c_string();
    return m;
}


matteVM_t * matte_get_vm(matte_t * m) {
    return m->vm;
}

void matte_destroy(matte_t * m) {
    matte_vm_destroy(m->vm);
    matte_syntax_graph_destroy(m->graph);
    matte_deallocate(m);
}

void * matte_get_user_data(matte_t * m) {
    return m->userdata;
}

void matte_set_user_data(matte_t * m, void * d) {
    m->userdata = d;
}



void matte_set_io(
    matte_t * m,
    char * (*input) (matte_t *),       
    void   (*output)(matte_t *, const char *),
    void   (*clear) (matte_t *)
) {
    
    m->input = input;
    m->output = output;
    m->clear = clear;
    if (!m->input) m->input = default_input;
    if (!m->output) m->output = default_output;
    if (!m->clear) m->clear = default_clear;
    matte_vm_set_print_callback(m->vm, default_print_callback, m);
    matte_vm_set_unhandled_callback(m->vm, default_unhandled_error, m);
}




const char * matte_introspect_value(matte_t * m, matteValue_t val) {
    if (m->introspectResult)
        matte_string_destroy(m->introspectResult);

    matteValue_t v = matte_run_source_with_parameters(m, "return import(module:'Matte.Core.Introspect')(value:parameters);", val);
    if (matte_value_type(v) == MATTE_VALUE_TYPE_STRING) {
        m->introspectResult = matte_string_clone(
            matte_value_string_get_string_unsafe(
                matte_vm_get_store(m->vm),
                v
            )
        );
        return matte_string_get_c_str(m->introspectResult);
    }
    return "";
}






void matte_add_external_function(
    // The embedding
    matte_t * m, 
    const char * name, 
    matteValue_t (*extFn)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),
    void * userData,
    ... 
) {
    matteString_t * id = matte_string_create_from_c_str("%s", name);
    va_list args;
    va_start(args, userData);
    
    
    matteArray_t * names = matte_array_create(sizeof(matteString_t *));
    const char * str;
    while((str = va_arg(args, char *))) {
        matteString_t * strm = matte_string_create_from_c_str("%s", str);
        matte_array_push(names, strm);
    }
    
    va_end(args);
    
    matte_vm_set_external_function(m->vm, id, names, extFn, userData);
    
    matte_string_destroy(id);
    uint32_t i;
    uint32_t len = matte_array_get_size(names);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(names, matteString_t *, i));
    }
    matte_array_destroy(names);
}


matteValue_t matte_call(
    matte_t * m, 
    matteValue_t func, 
    ... 
) {
    va_list args;
    va_start(args, func);
    
    matteStore_t * store = matte_vm_get_store(m->vm);
    matteArray_t * names = matte_array_create(sizeof(matteValue_t));
    matteArray_t * vals  = matte_array_create(sizeof(matteValue_t));

    const char * str;
    matteValue_t val;
    while((str = va_arg(args, char *))) {
        matteString_t * strm = matte_string_create_from_c_str("%s", str);
        matteValue_t strmval = matte_store_new_value(store);
        matte_value_into_string(store, &strmval, strm);
        matte_string_destroy(strm);
        val = va_arg(args, matteValue_t);    
        matte_array_push(names, strmval);
        matte_array_push(vals, val);
    }
    
    va_end(args);
    
    matteValue_t result = matte_vm_call(m->vm, func, vals, names, MATTE_VM_STR_CAST(m->vm, ""));
    
    matte_array_destroy(names);
    matte_array_destroy(vals);
    return result;
}


matteValue_t matte_run_source(matte_t * m, const char * source) {
    return matte_run_source_with_parameters(m, source, matte_store_new_value(matte_vm_get_store(m->vm)));
}




void matte_set_importer(
    matte_t * m,
    uint32_t(*importer)(matte_t *, const char * name, const char * alias, void *),
    void * userData
) {
    if (importer == NULL) {
        importer = default_importer;
    }
    m->importerData = userData;
    m->importer = importer;
    matte_vm_set_import(m->vm, matte_importer, m);
}


matteValue_t matte_run_bytecode(matte_t * m, const uint8_t * bytecode, uint32_t bytecodeSize) {
    return matte_run_bytecode_with_parameters(m, bytecode, bytecodeSize, matte_store_new_value(matte_vm_get_store(m->vm)));
}


uint8_t * matte_compile_source(matte_t * m, uint32_t * bytecodeSize, const char * source, matteString_t * error) {
    uint8_t * a = matte_compiler_run(
        m->graph,
        (uint8_t*)source,
        strlen(source),
        bytecodeSize,
        
        default_compile_error,
        m
    );
    if (error && a == NULL && matte_string_get_length(m->lastCompilerError)) {
        matte_string_set(error, m->lastCompilerError);
    }
    return a;
}


matteValue_t matte_run_bytecode_with_parameters(matte_t * m, const uint8_t * bytecode, uint32_t bytecodeSize, matteValue_t input) {

    matteString_t * fname = matte_string_create_from_c_str("[matte_run_source:%d]", m->runSourceCount++);
    uint32_t fid = matte_vm_get_new_file_id(m->vm, fname);

    matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
        matte_vm_get_store(m->vm),
        fid,
        bytecode,
        bytecodeSize
    );

    matte_vm_add_stubs(m->vm, stubs);

    
    matteValue_t out = matte_vm_run_fileid(
        m->vm,
        fid,
        input
    );
    
    matte_string_destroy(fname);
    return out;
}



matteValue_t matte_run_source_with_parameters(matte_t * m, const char * source, matteValue_t input) {
    
    uint32_t bytecodeSize;
    uint8_t * bytecode = matte_compiler_run(
        m->graph,
        (uint8_t*)source,
        strlen(source),
        &bytecodeSize,
        
        default_compile_error,
        m
    );
    if (!bytecodeSize || !bytecode) {
        return matte_store_new_value(matte_vm_get_store(m->vm));
    }
    
    
    matteString_t * fname = matte_string_create_from_c_str("[matte_run_source:%d]", m->runSourceCount++);
    uint32_t fid = matte_vm_get_new_file_id(m->vm, fname);

    matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
        matte_vm_get_store(m->vm),
        fid,
        bytecode,
        bytecodeSize
    );

    matte_vm_add_stubs(m->vm, stubs);
    matte_deallocate(bytecode);

    
    if (m->isDebug) {
        debug_split_lines(m, fid, (const uint8_t*)source, strlen(source));
    }
    
    matteValue_t out = matte_vm_run_fileid(
        m->vm,
        fid,
        input
    );
    
    matte_string_destroy(fname);
    
    return out;
}


uint32_t matte_add_module(
    matte_t * m, 
    const char * name, 
    const uint8_t * bytes, 
    uint32_t bytelen
) {
    uint32_t fileid = matte_vm_get_new_file_id(m->vm, MATTE_VM_STR_CAST(m->vm, name));   
    // determine if bytecode or raw source.
    // handle bytecodecase
    if (bytelen >= 6 &&
        bytes[0] == 'M'  &&
        bytes[1] == 'A'  &&
        bytes[2] == 'T'  &&
        bytes[3] == 0x01 &&
        bytes[4] == 0x06 &&
        bytes[5] == 'B') {
        
        
        matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
            matte_vm_get_store(m->vm),
            fileid,
            bytes,
            bytelen
        );
        if (stubs) {
            matte_vm_add_stubs(m->vm, stubs);
        } else {
            fileid = 0; // failed.
            matteString_t * str = matte_string_create_from_c_str("Failed to assemble bytecode %s.", name);
            matte_vm_raise_error_string(m->vm, str);
            matte_string_destroy(str);
        }        
    // raw source
    } else {
        uint32_t bytecodeLen;
        uint8_t * bytecode = matte_compiler_run(
            m->graph,
            bytes,
            bytelen,
            &bytecodeLen,
            default_compile_error,            
            m
        );
        
        if (!bytes || ! bytecodeLen) {
            fileid = 0; // failed.
            matteString_t * str = matte_string_create_from_c_str("Could not import '%s'. %s", name, (m->lastCompilerError) ? matte_string_get_c_str(m->lastCompilerError) : "<no data>");
            matte_vm_raise_error_string(m->vm, str);
            matte_string_destroy(str);
        } else {
            if (m->isDebug)
                debug_split_lines(m, fileid, bytes, bytelen);
        
        
            matteArray_t * stubs = matte_bytecode_stubs_from_bytecode(
                matte_vm_get_store(m->vm),
                fileid,
                bytecode,
                bytecodeLen
            );
            if (stubs) {
                matte_vm_add_stubs(m->vm, stubs);
                matte_array_destroy(stubs);
                stubs = NULL;
            } else {
                fileid = 0; // failed.
                matteString_t * str = matte_string_create_from_c_str("Failed to assemble bytecode %s.", name);
                matte_vm_raise_error_string(m->vm, str);
                matte_string_destroy(str);
            }           
        }
        matte_deallocate(bytecode);
    }  
    return fileid;  
}





void matte_debugging_enable(matte_t * m) {
    if (m->isDebug) return;
    m->isDebug = 1;
    m->lines = matte_table_create_hash_pointer(); // needs editing
    matte_vm_set_debug_callback(
        m->vm,
        debug_on_event,
        m
    );
    m->lastInput = matte_strdup("");
}


void matte_debugging_register_source(
    matte_t * m,
    uint32_t fileID,
    const char * source        
) {
    debug_split_lines(
        m,
        fileID,
        (const uint8_t *) source,
        strlen(source)
    );
}


static void* alloc_default(uint64_t size) {
    return malloc(size);
}

void * matte_allocate(uint64_t size) {
    if (!matte_allocate_fn)
        matte_allocate_fn = alloc_default;
    if (!size) return NULL;
    uint8_t * data = (uint8_t*)matte_allocate_fn(size);
    if (!data) return data;
    memset(data, 0, size);
    return data;
}

void matte_deallocate(void * data) {
    if (!data) return;
    if (!matte_deallocate_fn)
        matte_deallocate_fn = free;
    matte_deallocate_fn(data);
}


void matte_set_allocator(
    void * (*allocator)  (uint64_t),
    void   (*deallocator)(void*)
) {
    matte_allocate_fn = allocator;
    matte_deallocate_fn = deallocator;
}

matteSyntaxGraph_t * matte_get_syntax_graph(matte_t * m) {
    return m->graph;
}





matteValue_t matte_load_package(
    matte_t * m,
    const uint8_t * packageBytes,
    uint32_t packageByteLength
) {
    const uint8_t * iter = packageBytes;
    uint32_t bytesLeft = packageByteLength;
    
    #define matte_load_package__read(__T__, __O__) \
        {\
        if (bytesLeft < (__T__)) {\
            matte_vm_raise_error_cstring(m->vm, "Package is truncated.");\
            goto L_FAIL;\
        }\
        memcpy(&(__O__), iter, (__T__));\
        iter += (__T__);\
        bytesLeft -= (__T__);\
        }
    
    
    while(bytesLeft) {
        if (packageByteLength < 7+4) {
            matte_vm_raise_error_cstring(m->vm, "Buffer is not a package.");
            return matte_store_new_value(matte_vm_get_store(m->vm));
        }

        if (!(iter[0] == 23 &&
              iter[1] == 14 &&
              iter[2] == 'M' &&
              iter[3] == 'P' &&
              iter[4] == 'K' &&
              iter[5] == 'G' &&
              iter[6] == 1)) { // we only support version 1 right now!
            matte_vm_raise_error_cstring(m->vm, "Package header is invalid.");
            return matte_store_new_value(matte_vm_get_store(m->vm));          
        }
        iter += 7;
        uint32_t size = *((uint32_t*)iter);
        iter += 4;
        bytesLeft -= 11;
        if (size > bytesLeft) {
            matte_vm_raise_error_cstring(m->vm, "Package header is truncated.");
            return matte_store_new_value(matte_vm_get_store(m->vm));              
        }
        
        // extract JSON
        uint8_t * utf8Buffer = NULL;
        uint8_t * dataBuffer = NULL;
        utf8Buffer = (uint8_t*)matte_allocate(size+1);
        memcpy(utf8Buffer, iter, size); 
        bytesLeft -= size;
        iter += size;   
        
        matteString_t * json = matte_string_create_from_c_str("%s", utf8Buffer);
        matte_deallocate(utf8Buffer);
        utf8Buffer = NULL;
        
        matteStore_t * store = matte_vm_get_store(m->vm);
        matteValue_t inputJson = matte_store_new_value(store);
        matte_value_into_string(store, &inputJson, json);
        matte_string_destroy(json);
        
        matteValue_t jsonObject = matte_run_source_with_parameters(
            m,
            "return (import(module:'Matte.Core.JSON')).decode(string:parameters);",
            inputJson
        );
        matte_value_object_push_lock(store, jsonObject); // keep
        
        // could not parse JSON
        if (matte_value_type(jsonObject) == MATTE_VALUE_TYPE_EMPTY) {
            return matte_store_new_value(store);
        }
        
        matteValue_t nextVal;
        matteValue_t nv1;
        
        
        
        mattePackageInfo_t * package = (mattePackageInfo_t*)matte_allocate(sizeof(mattePackageInfo_t));
        // extract name and version info from package 
        package->packageJson = jsonObject;
        
        nextVal = matte_value_object_access_string(store, jsonObject, MATTE_VM_STR_CAST(m->vm, "name"));
        if (matte_value_type(nextVal) == MATTE_VALUE_TYPE_EMPTY) {
            matte_vm_raise_error_cstring(m->vm, "Package is missing name parameter.");
            goto L_FAIL;
        }
        nextVal = matte_value_as_string(store, nextVal);
        if (matte_value_type(nextVal) == MATTE_VALUE_TYPE_EMPTY) {
            matte_vm_raise_error_cstring(m->vm, "Package name parameter is not a string.");
            goto L_FAIL;
        }
        package->name = matte_string_clone(matte_value_string_get_string_unsafe(store, nextVal));
        
        
        nextVal = matte_value_object_access_string(store, jsonObject, MATTE_VM_STR_CAST(m->vm, "version"));
        if (matte_value_type(nextVal) != MATTE_VALUE_TYPE_OBJECT) {
            matte_vm_raise_error_cstring(m->vm, "Package version isnt an object.");
            goto L_FAIL;    
        }

        if (matte_value_object_get_number_key_count(store, nextVal) < 3) {
            matte_vm_raise_error_cstring(m->vm, "Package version should contain at least 3 values.");
            goto L_FAIL;    
        }
        
        nv1 = matte_value_object_access_index(store, nextVal, 0);
        if (matte_value_type(nv1) != MATTE_VALUE_TYPE_NUMBER) {
            matte_vm_raise_error_cstring(m->vm, "Package major version MUST be a number.");
            goto L_FAIL;            
        }
        package->versionMajor = matte_value_as_number(store, nv1);

        nv1 = matte_value_object_access_index(store, nextVal, 1);
        if (matte_value_type(nv1) != MATTE_VALUE_TYPE_NUMBER) {
            matte_vm_raise_error_cstring(m->vm, "Package minor version MUST be a number.");
            goto L_FAIL;            
        }
        package->versionMinor = matte_value_as_number(store, nv1);






        // next, load all sources
        uint32_t sourceCount;
        matte_load_package__read(sizeof(uint32_t), sourceCount);
        
        
        uint32_t i;
        for(i = 0; i < sourceCount; ++i) {
            uint32_t bufferLength;
            matte_load_package__read(sizeof(uint32_t), bufferLength);
            utf8Buffer = (uint8_t*)matte_allocate(bufferLength+1);
            matte_load_package__read(bufferLength, utf8Buffer[0]);

            uint32_t dataLength;
            matte_load_package__read(sizeof(uint32_t), dataLength);
            dataBuffer = (uint8_t*)matte_allocate(dataLength);
            matte_load_package__read(dataLength, dataBuffer[0]);
            
            
            matteString_t * str;
            
            if (!strcmp((char*)utf8Buffer, "main.mt")) {
                str = matte_string_clone(package->name);
            } else {
                str = matte_string_create_from_c_str(
                    "%s.%s", 
                    matte_string_get_c_str(package->name),
                    utf8Buffer
                );
            }
            matte_add_module(
                m,
                matte_string_get_c_str(str),
                dataBuffer,
                dataLength
            );
        }

        uint8_t more;
        matte_load_package__read(1, more);

        matte_table_insert(
            m->packages,
            matte_string_get_c_str(package->name),
            package
        );

        if (!more) break;
        continue;
      L_FAIL:
        if (utf8Buffer)
            matte_deallocate(utf8Buffer);
        if (package->name)
            matte_string_destroy(package->name);
        matte_value_object_pop_lock(store, jsonObject);
        matte_deallocate(package);
        return matte_store_new_value(store);

        
    }
    matteStore_t * store = matte_vm_get_store(m->vm);
    return matte_store_new_value(store);
    
}


matteValue_t matte_get_package_info(
    matte_t * m,
    const char * packageName
) {
    
    mattePackageInfo_t * package = (mattePackageInfo_t*)matte_table_find(
        m->packages,
        packageName
    );
    if (package == NULL) return matte_store_new_value(matte_vm_get_store(m->vm));
    
    return package->packageJson;
}
    

int matte_check_package(
    matte_t * m,
    const char * packageName
) {
    matteStore_t * store = matte_vm_get_store(m->vm);
    mattePackageInfo_t * package = (mattePackageInfo_t*)matte_table_find(
        m->packages,
        packageName
    );
    if (package == NULL) {
        matteString_t * errms = matte_string_create_from_c_str("Couldn't find package %s", packageName);
        matte_vm_raise_error_string(m->vm, errms);
        matte_string_destroy(errms);
        return 0;
    }

    matteValue_t depends = matte_value_object_access_string(store, package->packageJson, MATTE_VM_STR_CAST(m->vm, "depends"));
    if (matte_value_type(depends) != MATTE_VALUE_TYPE_OBJECT) {
        matteString_t * errms = matte_string_create_from_c_str("Package %s is malformed (missing depends attribute)", packageName);
        matte_vm_raise_error_string(m->vm, errms);
        matte_string_destroy(errms);
        return 0;    
    }
    
    
    
    uint32_t i;
    uint32_t len = matte_value_object_get_number_key_count(store, package->packageJson);
    for(i = 0; i < len; ++i) {
        matteValue_t dependency = matte_value_object_access_index(store, depends, i);
        if (matte_value_type(dependency) != MATTE_VALUE_TYPE_OBJECT ||
            matte_value_object_get_number_key_count(store, dependency) < 2) {
            matteString_t * errms = matte_string_create_from_c_str("Package %s is malformed (dependency %d is not an array of at least 2)", packageName, (int)i);
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;                
        }
        
        matteValue_t name = matte_value_object_access_index(store, dependency, 0);
        if (matte_value_type(name) != MATTE_VALUE_TYPE_STRING) {
            matteString_t * errms = matte_string_create_from_c_str("Package %s is malformed (dependency %d's name is not a string)", packageName, (int)i);
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;
        }
        
        const matteString_t * nameStr = matte_value_string_get_string_unsafe(store, name);
        mattePackageInfo_t * pkgInfo = (mattePackageInfo_t*)matte_table_find(
            m->packages,
            matte_string_get_c_str(nameStr)
        );
        
        
        if (pkgInfo == NULL) {
            matteString_t * errms = matte_string_create_from_c_str("Package %s missing dependency '%s'", packageName, matte_string_get_c_str(nameStr));
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;        
        }
        
        
        matteValue_t versionRequirement = matte_value_object_access_index(store, dependency, 1);
        if (matte_value_type(versionRequirement) != MATTE_VALUE_TYPE_OBJECT ||
            matte_value_object_get_number_key_count(store, versionRequirement) < 2) {
            matteString_t * errms = matte_string_create_from_c_str("Package %s is malformed (dependency %d's version requirement is not an array of at least 2)", packageName, (int)i);
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;        
        }
        
        matteValue_t majorVersionVal = matte_value_object_access_index(store, versionRequirement, 0);
        matteValue_t minorVersionVal = matte_value_object_access_index(store, versionRequirement, 1);
        
        if (matte_value_type(majorVersionVal) != MATTE_VALUE_TYPE_NUMBER||
            matte_value_type(minorVersionVal) != MATTE_VALUE_TYPE_NUMBER) {
            matteString_t * errms = matte_string_create_from_c_str("Package %s is malformed (dependency %d's version requirement does not consist of a major and minor number)", packageName, (int)i);
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;                    
        }
        
        int majorVersion = matte_value_as_number(store, majorVersionVal);
        int minorVersion = matte_value_as_number(store, minorVersionVal);
        
        if (majorVersion > pkgInfo->versionMajor ||
            (majorVersion == pkgInfo->versionMajor &&
             minorVersion > pkgInfo->versionMinor)) {

            matteString_t * errms = matte_string_create_from_c_str("Package %s requires package \"%s\"  v%d.%d, but the currently loaded version is %d.%d", 
                packageName, matte_string_get_c_str(nameStr),
                majorVersion, minorVersion,
                pkgInfo->versionMajor, pkgInfo->versionMinor                
            );
            matte_vm_raise_error_string(m->vm, errms);
            matte_string_destroy(errms);
            return 0;                                             
        }
    }
    return 1;
}





matteArray_t * matte_get_package_dependencies(
    matte_t * m,
    const char * packageName
) {
    matteStore_t * store = matte_vm_get_store(m->vm);
    mattePackageInfo_t * package = (mattePackageInfo_t*)matte_table_find(
        m->packages,
        packageName
    );
    if (package == NULL) {
        return NULL;
    }

    matteValue_t depends = matte_value_object_access_string(store, package->packageJson, MATTE_VM_STR_CAST(m->vm, "depends"));
    if (matte_value_type(depends) != MATTE_VALUE_TYPE_OBJECT) {
        return NULL;    
    }
    
    
    
    uint32_t i;
    uint32_t len = matte_value_object_get_number_key_count(store, package->packageJson);

    matteArray_t * output = matte_array_create(sizeof(matteString_t *));

    for(i = 0; i < len; ++i) {
        matteValue_t dependency = matte_value_object_access_index(store, depends, i);
        if (matte_value_type(dependency) != MATTE_VALUE_TYPE_OBJECT ||
            matte_value_object_get_number_key_count(store, dependency) < 2) {
            goto L_FAIL;
        }
        
        matteValue_t name = matte_value_object_access_index(store, dependency, 0);
        if (matte_value_type(name) != MATTE_VALUE_TYPE_STRING) {
            goto L_FAIL;
        }
        
        matteString_t * nameStr = matte_string_clone(matte_value_string_get_string_unsafe(store, name));
        matte_array_push(output, nameStr);
    }
    
    return output;
    
  L_FAIL:
    i = 0;
    len = matte_array_get_size(output);
    for(; i < len; ++i) {
        matteString_t * str = matte_array_at(output, matteString_t *, i);
        matte_string_destroy(str);    
    }
    matte_array_destroy(output);
    return NULL;
}



