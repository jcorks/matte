#include <emscripten.h>
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

EMSCRIPTEN_KEEPALIVE
const char * matte_js_syntax_analysis(const char * source) {
    static char * output = NULL;
    uint32_t len = strlen(source);
    matteString_t * result = matte_compiler_tokenize(
        (uint8_t*)source,
        len,
        NULL,
        NULL
    );

    free(output); output = NULL;
    if (!result) {
        return "<Error while parsing source. Please check!>";
    }


    output = strdup(matte_string_get_c_str(result));
    matte_string_destroy(result);
    return output;
}



static matteString_t * matte_js_run__stdout = NULL;



static void matte_js_run__print(matteVM_t * vm, const matteString_t * str, void * ud) {
    matte_string_concat_printf(
        matte_js_run__stdout,
        "%s\n", matte_string_get_c_str(str)
    );
}



static void matte_js_run__unhandled_error(
    matteVM_t * vm, 
    uint32_t file, 
    int lineNumber, 
    matteValue_t val,
    void * d
) {
    if (val.binID == MATTE_VALUE_TYPE_OBJECT) {
        matteValue_t s = matte_value_object_access_string(matte_vm_get_heap(vm), val, MATTE_VM_STR_CAST(vm, "summary"));
        if (s.binID) {
            
            matte_string_concat_printf(
                matte_js_run__stdout,            
                "Unhandled error: %s\n", 
                matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), s))
            );
            return;
        }
    }
    
    matte_string_concat_printf(
        matte_js_run__stdout,
        matte_string_get_c_str(matte_vm_get_script_name_by_id(vm, file)), 
        lineNumber
    );
}



static void matte_js_run__on_error(const matteString_t * s, uint32_t line, uint32_t ch, void * userdata) {    
    matte_string_concat_printf(
        matte_js_run__stdout,
        "Could not compile source:\n%s (line %d:%d)\n", matte_string_get_c_str(s), line, ch
    );
}

EMSCRIPTEN_KEEPALIVE
const char * matte_js_run(const char * source) {
    if (!matte_js_run__stdout) matte_js_run__stdout = matte_string_create();
    matte_string_clear(matte_js_run__stdout);

    matte_t * m = matte_create();
    matteVM_t * vm = matte_get_vm(m);

    matte_vm_set_print_callback(vm, matte_js_run__print, NULL);
    matte_vm_set_unhandled_callback(vm, matte_js_run__unhandled_error, NULL);


    uint32_t outByteLen;
    uint32_t sourceLen = strlen(source);
    uint8_t * outBytes = matte_compiler_run(
        (uint8_t*)source,
        sourceLen,
        &outByteLen,
        matte_js_run__on_error,
        NULL
    );

    if (!outBytes) {
        return matte_string_get_c_str(matte_js_run__stdout);
    }

    uint32_t fileID = matte_vm_get_new_file_id(vm, MATTE_VM_STR_CAST(vm, "source"));
    matteArray_t * arr = matte_bytecode_stubs_from_bytecode(
        matte_vm_get_heap(vm), 
        fileID, 
        outBytes, 
        outByteLen
    );
    if (arr) {
        matte_vm_add_stubs(vm, arr);
    } else {
        return matte_string_get_c_str(matte_js_run__stdout);
    }
    
    matteValue_t v = matte_vm_run_fileid(vm, fileID, matte_heap_new_value(matte_vm_get_heap(vm)), NULL);
    matte_string_concat_printf(
        matte_js_run__stdout,
        "> %s\n", 
            matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_heap(vm), matte_value_as_string(matte_vm_get_heap(vm), v)))
    );
    matte_heap_recycle(matte_vm_get_heap(vm), v);
    matte_destroy(m);
    return matte_string_get_c_str(matte_js_run__stdout);
}
