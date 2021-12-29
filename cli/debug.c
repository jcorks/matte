#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include "../src/matte_array.h"
#include "../src/matte_string.h"
#include "../src/matte_table.h"
#include "../src/matte_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "shared.h"

#define MESSAGE_LEN_MAX 256

static char * prompt() {
    printf("(mdb) ");
    fflush(stdout);
    
    static char buff[MESSAGE_LEN_MAX+1];
    fgets(buff, MESSAGE_LEN_MAX, stdin);
    return buff;
}

static int keepgoing = 1;
static int started = 0;
static matteVM_t * vm = NULL;
static matteArray_t * breakpoints = NULL;
static int stepinto = 0;
static int stepreqframe = 0;
static char * lastCommand = NULL;
// fileid -> matteArray -> matteString_t *
static matteTable_t * lines;
static int stackframe = 0;
uint32_t DEBUG_FILEID;

typedef struct {
    uint32_t fileid;
    uint32_t line;
} breakpoint;

static void split_lines(uint32_t fileid, const uint8_t * data, uint32_t size) {
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
    matte_table_insert_by_uint(lines, fileid, localLines);
}

static void onError(const matteString_t * s, uint32_t line, uint32_t ch, void * nu) {
    printf("%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch);
    fflush(stdout);
}

static void printCommandError(
    matteVM_t * vm, 
    matteVMDebugEvent_t event, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * data
) {
    if(event == MATTE_VM_DEBUG_EVENT__ERROR_RAISED)
        printf("Error while evaluating print command: %s\n", matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), matte_value_as_string(matte_vm_get_heap(vm), val))));
}

#define PRINT_AREA_LINES 20
static void printArea() {
    matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, stackframe);
    uint32_t fileid = matte_bytecode_stub_get_file_id(frame.stub);
    uint32_t numinst;
    const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);
    uint32_t line = 0;
    if (frame.pc-1 >= 0 && frame.pc-1 < numinst)
        line = inst[frame.pc-1].lineNumber;

    // ansi clear screen
    printf("\x1b[2J");
    printf("<file %s, line %d>\n", 
        matte_vm_get_script_name_by_id(vm, fileid) ? 
            matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, fileid)) 
        : 
            "???" , 
        line
    );
    int i = line;

    matteArray_t * localLines = matte_table_find_by_uint(lines, fileid);
    if (localLines) {
        for(i = ((int)line) - PRINT_AREA_LINES/2; i < ((int)line) + PRINT_AREA_LINES/2 + 1; ++i) {
            if (i < 0 || i >= matte_array_get_size(localLines)) {
                printf("  ---- | \n");
            } else {
                if (i == line-1) {
                    printf("->%4d | %s\n", i+1, matte_string_get_c_str(matte_array_at(localLines, matteString_t *, i)));
                } else {
                    printf("  %4d | %s\n", i+1, matte_string_get_c_str(matte_array_at(localLines, matteString_t *, i)));
                }
            }
        }
    }
    printf("\n");
    fflush(stdout);
}

static int execCommand() {
    static char command[MESSAGE_LEN_MAX+1];
    char * res = prompt();
    if (res[0] == '\n') {
        strcpy(res, lastCommand);
    } else {
        free(lastCommand);
        lastCommand = strdup(res);
    }

    char * iter = command;
    while(*res && !isspace(*res)) {
        *iter = *res;
        iter++; 
        res++;
    }
    *iter = 0;

    if (!strcmp(command, "break")||
        !strcmp(command, "b")) {
        breakpoint p;
        int line;
        if (sscanf(res, "%d", &line) == 1) {
            p.fileid = 1;
            p.line = line;
            printf("Added breakpoint %d\n", matte_array_get_size(breakpoints));
            matte_array_push(breakpoints, p);
        } else {
            printf("Could not recognize line.\n");
        }
    } else if (!strcmp(command, "run") ||
               !strcmp(command, "r")) {
        if (!started) {
            started = 1;
            matte_vm_run_fileid(vm, DEBUG_FILEID, matte_heap_new_value(matte_vm_get_heap(vm)));


            printf("Execution complete.\n");
            started = 0;    

        } else {
            printf("The script is already running.\n");            
        }
    } else if (!strcmp(command, "delete") ||
               !strcmp(command, "d")) {
        if (matte_array_get_size(breakpoints)) {
            printf("Remove all breakpoints? (Y/n)\n");
            char * res = prompt();
            if (res[0] == 'Y') { 
                matte_array_set_size(breakpoints, 0);
                printf("Breakpoints cleared.\n");
            }
        } else {
            printf("No breakpoints saved.\n");
        }
    } else if (!strcmp(command, "exit")) {
        printf("Are you sure you want to quit? (Y/n)\n");
        char * res = prompt();
        if (res[0] == 'Y') { 
            exit(0);
        }
    } else if (!strcmp(command, "step") ||
               !strcmp(command, "s")) {
        if (started) {
            stepinto = 1;
            stepreqframe = -1;
            return 0;
        } else {
            printf("The script has not started yet.\n");
        }
    } else if (!strcmp(command, "next") ||
               !strcmp(command, "n")) {
        if (started) {
            stepinto = 1;
            stepreqframe = matte_vm_get_stackframe_size(vm);
            return 0;
        } else {
            printf("The script has not started yet.\n");
        }
    } else if (!strcmp(command, "continue")||
               !strcmp(command, "c")) {
        if (!started) {
            printf("The script is not currently running.\n");
        } else {
            return 0;
        }
    } else if (!strcmp(command, "backtrace") ||
               !strcmp(command, "bt")) {
        if (started) {
            uint32_t i;
            uint32_t len = matte_vm_get_stackframe_size(vm);
            for(i = 0; i < len; ++i) {
                matteVMStackFrame_t frame = matte_vm_get_stackframe(vm, i);
                uint32_t numinst;
                const matteBytecodeStubInstruction_t * inst = matte_bytecode_stub_get_instructions(frame.stub, &numinst);

                uint32_t fileid = matte_bytecode_stub_get_file_id(frame.stub);
                const matteString_t * fileName = matte_vm_get_script_name_by_id(vm, fileid);

                if (i == stackframe) {
                    printf(" -> @%d:%s <%s>, line %d\n", i, matte_string_get_c_str(frame.prettyName), fileName ? matte_string_get_c_str(fileName) : "???", inst[frame.pc].lineNumber);
                } else {
                    printf("    @%d:%s <%s>, line %d\n", i, matte_string_get_c_str(frame.prettyName), fileName ? matte_string_get_c_str(fileName) : "???", inst[frame.pc].lineNumber);
                }
            }

        } else {
            printf("The script has not started yet.\n");
        }
    } else if (!strcmp(command, "up") ||    
               !strcmp(command, "u")) {
        if (stackframe < matte_vm_get_stackframe_size(vm) - 1) {
            stackframe++;
            printArea();
        } else {
            printf("Already at top of callstack.\n");
        }
    } else if (!strcmp(command, "down") ||    
               !strcmp(command, "d")) {
        if (stackframe > 0) {
            stackframe--;
            printArea();
        } else {
            printf("Already at bottom of callstack.\n");
        }
    } else if (!strcmp(command, "print") ||    
               !strcmp(command, "p")) {

        matteString_t * src = matte_string_create();
        matte_string_concat_printf(src, "return import(module:'debug.mt').printObject(o:%s);", res);


        matteValue_t output = matte_vm_run_scoped_debug_source(
            vm,
            src,
            stackframe,
            printCommandError,
            NULL
        );

        const matteString_t * str = matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), matte_value_as_string(matte_vm_get_heap(vm), output));
        if (str) {
            printf("%s\n", matte_string_get_c_str(str));            
        }
        matte_string_destroy(src);  

        matte_heap_recycle(matte_vm_get_heap(vm), output);

    } else {
        printf("Unknown command.\n");        
    }
    return 1;
}



static void onDebugEvent(
    matteVM_t * vm, 
    matteVMDebugEvent_t event, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * data
) {
    if (event == MATTE_VM_DEBUG_EVENT__ERROR_RAISED) {
        // error happened before stackframe could start, so its not super useful to start at 0
        if (matte_vm_get_stackframe(vm, stackframe).pc == 0)
            stackframe++;

        printArea();
        const matteString_t * str = matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), matte_value_as_string(matte_vm_get_heap(vm), val));
        printf("ERROR RAISED FROM VIRTUAL MACHINE: %s\n", str ? matte_string_get_c_str(str) : "<no string info given>");
        while(execCommand());
        return;    
    }
     
    if (stepinto &&
        (stepreqframe == -1 ||
         matte_vm_get_stackframe_size(vm) <= stepreqframe)
    ) {
        stackframe = 0;
        stepinto = 0;
        printArea();
        while(execCommand());
        return;
    }
    uint32_t i;
    for(i = 0; i < matte_array_get_size(breakpoints); ++i) {
        breakpoint p = matte_array_at(breakpoints, breakpoint, i);
        if (p.fileid == file &&
            p.line == lineNumber) {
            stackframe = 0;
            printf("Breakpoint %d reached.\n", i);
            printArea();
            while(execCommand());
        }
    }
}


static void onDebugPrint(matteVM_t * vm, const matteString_t * str, void * ud) {
    printf("%s\n", matte_string_get_c_str(str));
}



int matte_debug(const char * input, char ** argv, int argc) {
    matte_t * m = matte_create();
    vm = matte_get_vm(m);
    DEBUG_FILEID = matte_vm_get_new_file_id(vm, MATTE_VM_STR_CAST(vm, input));
    matte_vm_set_debug_callback(vm, onDebugEvent, NULL);
    matte_vm_set_print_callback(vm, onDebugPrint, NULL);
    printf("Compiling %s...\n", input);
    fflush(stdout);    
    uint32_t lenBytes;
    uint8_t * src = dump_bytes(input, &lenBytes);
    if (!src || !lenBytes) {
        printf("Couldn't open source %s\n", input);
        fflush(stdout);    
        exit(1);
    }
    lines = matte_table_create_hash_pointer();
    matte_table_insert_by_uint(lines, 0, matte_array_create(sizeof(matteString_t *)));
    split_lines(1, src, lenBytes);



    uint32_t outByteLen;
    uint8_t * outBytes = matte_compiler_run(
        src,
        lenBytes,
        &outByteLen,
        onError,
        NULL
    );


    free(src);
    if (!outByteLen || !outBytes) {
        printf("Couldn't compile source %s\n", input);
        exit(1);
    }
    lastCommand = strdup("");
    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(matte_vm_get_heap(vm), DEBUG_FILEID, outBytes, outByteLen);
    matte_vm_add_stubs(vm, arr);
    printf("...Done! (%.2fKB to %.2fKB)\n\n", lenBytes / 1000.0, outByteLen / 1000.0);

    printf("Welcome to the Matte debugger!\n");
    printf("Enter 'help' for available commands.\n");
    fflush(stdout);



    breakpoints = matte_array_create(sizeof(breakpoint));
    while(keepgoing) execCommand();
    printf("Exiting.\n");
    fflush(stdout);
    return 0;

}
