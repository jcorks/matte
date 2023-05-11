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


struct matte_t {
    matteVM_t * vm;
    void * userdata;

    // The assigned input function. Always populated
    char * (*input) (matte_t *);
    
    // The assigned output function. Always populated
    void   (*output)(matte_t *, const char *);
    
    // Function to clear output. Always populated
    void   (*clear)(matte_t *);
    
    // cached syntax graph.
    matteSyntaxGraph_t * graph;
    
    // whether the debug context is enabled.
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
    
    // for matte_print(), formatting output buffer.
    char * formatBuffer;
    
    // result of last introspect.
    matteString_t * introspectResult;
    
    // List of breakpoints. currently not used.
    matteArray_t * breakpoints;
    
    // current stackframe in debugger
    uint32_t stackframe;
    
    // Last prompt input
    char * lastInput;
    
    // Current prompt input
    char * currentInput;
};


// Convenience function for printf style print to 
// output function for matte instance
#define MATTE_PRINT_LIMIT 4096
static void matte_print(matte_t * m, const char * fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(
        m->formatBuffer,
        MATTE_PRINT_LIMIT,
        fmt,
        args
    );
    m->output(m, m->formatBuffer);
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
        line = inst[frame.pc-1].lineNumber;

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
    matte_print(m, "%s (line %d:%d)", matte_string_get_c_str(str), line, ch);
    
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
        m->currentInput = strdup(m->lastInput);
    } else {
        matte_deallocate(m->lastInput);
        m->lastInput = strdup(m->currentInput);
    }

    // command is the first word
    char * iter = strdup(m->currentInput);
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

            if (i == m->stackframe) {
                matte_print(m, " -> @%d: <%s>, line %d", i, fileName ? matte_string_get_c_str(fileName) : "???", inst[frame.pc].lineNumber);
            } else {
                matte_print(m, "    @%d: <%s>, line %d", i, fileName ? matte_string_get_c_str(fileName) : "???", inst[frame.pc].lineNumber);
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


static uint8_t * debug_on_import(
    matteVM_t * vm,
    const matteString_t * importPath,
    uint32_t * preexistingFileID,
    uint32_t * dataLength,
    void * usrdata
) {
    uint8_t * out = matte_vm_get_default_import()(
        vm,
        importPath,
        preexistingFileID,
        dataLength,
        usrdata
    );

    debug_split_lines((matte_t*)usrdata, *preexistingFileID, out, *dataLength);
    return out;
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
    if (val.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t s = matte_value_object_access_string(matte_vm_get_store(vm), val, MATTE_VM_STR_CAST(vm, "summary"));
        if (s.binID && s.binID == MATTE_VALUE_TYPE_STRING) {
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






matte_t * matte_create() {
    matte_t * m = (matte_t*)matte_allocate(1*sizeof(matte_t));
    m->output = default_output;
    m->input = default_input;
    m->formatBuffer = (char *)matte_allocate(MATTE_PRINT_LIMIT+1);
    m->graph = matte_syntax_graph_create();
    m->vm = matte_vm_create(m);
    return m;
}


matteVM_t * matte_get_vm(matte_t * m) {
    return m->vm;
}

void matte_destroy(matte_t * m) {
    matte_vm_destroy(m->vm);
    matte_syntax_graph_destroy(m->graph);
    matte_deallocate(m->formatBuffer);
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
    m->introspectResult = matte_string_clone(
        matte_value_string_get_string_unsafe(
            matte_vm_get_store(m->vm),
            matte_run_source_with_parameters(m, "return import(module:'Matte.Core.Introspect')(value:parameters);", val)
        )
    );
    return matte_string_get_c_str(m->introspectResult);
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

matteValue_t matte_run_bytecode(matte_t * m, const uint8_t * bytecode, uint32_t bytecodeSize) {
    return matte_run_bytecode_with_parameters(m, bytecode, bytecodeSize, matte_store_new_value(matte_vm_get_store(m->vm)));
}


uint8_t * matte_compile_source(matte_t * m, uint32_t * bytecodeSize, const char * source) {
    return matte_compiler_run(
        m->graph,
        (uint8_t*)source,
        strlen(source),
        bytecodeSize,
        
        default_compile_error,
        m
    );
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
        input,
        fname
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
        // do split manually since will bypass normal import 
        debug_split_lines(m, fid, (const uint8_t*)source, strlen(source));
    }
    
    matteValue_t out = matte_vm_run_fileid(
        m->vm,
        fid,
        input,
        fname
    );
    
    matte_string_destroy(fname);
    
    return out;
}






void matte_debugging_enable(matte_t * m) {
    m->isDebug = 1;
    m->lines = matte_table_create_hash_pointer(); // needs editing
    matte_vm_set_import(m->vm, debug_on_import, m);
    matte_vm_set_debug_callback(
        m->vm,
        debug_on_event,
        m
    );
    m->lastInput = strdup("");
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


