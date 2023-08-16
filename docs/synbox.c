#include <emscripten.h>
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include "../src/matte_compiler__syntax_graph.h"
#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>



//EMSCRIPTEN_KEEPALIVE
const char * matte_js_syntax_analysis(const char * source) {
    static matteSyntaxGraph_t * graph = NULL;
    static char * output = NULL;
    
    if (graph == NULL)
        graph = matte_syntax_graph_create();
    
    uint32_t len = strlen(source);
    matteString_t * result = matte_compiler_tokenize(
        graph,
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
static matteString_t * matte_js_run__source = NULL;


static void matte_js_run__print(matte_t * vm, const char * str) {
    matte_string_concat_printf(
        matte_js_run__stdout,
        "%s\n", str
    );
}

//EMSCRIPTEN_KEEPALIVE
const char * matte_js_repl(const char * input) {
    static matte_t * m = NULL;
    static matteString_t * output = NULL;
    static matteValue_t state;
    if (m == NULL) {
        matte_js_run__stdout = matte_string_create();
        m = matte_create();
        output = matte_string_create();
        matte_set_io(m, NULL, matte_js_run__print, NULL);    

        matteVM_t * vm = matte_get_vm(m);
        matteStore_t * store = matte_vm_get_store(vm);
        state = matte_store_new_value(store);

        matte_value_into_new_object_ref(store, &state);
        matte_value_object_push_lock(store, state);
        
        
        matte_string_concat_printf(output, "Matte REPL.\n");
        matte_string_concat_printf(output, "Johnathan Corkery, 2023\n");
        matte_string_concat_printf(output, "REPL: 'store' variable exists to store values.\n\n");
        return matte_string_get_c_str(output);
    }

    matte_string_clear(output);
    matte_string_clear(matte_js_run__stdout);

    matte_string_concat_printf(output, ">> %s\n", input);
        
        
    matteString_t * source = matte_string_create_from_c_str("return ::(store){return (%s);};", input);
    matteValue_t nextFunc = matte_run_source(m, matte_string_get_c_str(source));
    matte_string_destroy(source);

    if (matte_value_type(nextFunc) != MATTE_VALUE_TYPE_EMPTY) {
        matteValue_t result = matte_call(
            m,
            nextFunc,
            "store", state,
            NULL
        );
        matte_string_concat_printf(output, "%s\n\n", matte_introspect_value(m, result));        
    }
    
    if (matte_string_get_length(matte_js_run__stdout) > 0)
        matte_string_concat_printf(output, "From Matte: %s", matte_string_get_c_str(matte_js_run__stdout));
    
    return matte_string_get_c_str(output);
}






//EMSCRIPTEN_KEEPALIVE
const char * matte_js_run(const char * source) {
    if (!matte_js_run__stdout) {
        matte_js_run__stdout = matte_string_create();
        matte_js_run__source = matte_string_create();
    }
    matte_string_clear(matte_js_run__stdout);
    matte_string_clear(matte_js_run__source);

    matte_string_concat_printf(matte_js_run__source, "%s", source);

    matte_t * m = matte_create();

    matte_set_io(m, NULL, matte_js_run__print, NULL);

    int err = 0;
    matteString_t * src = matte_string_create_from_c_str(
        "@val__ = ::<={%s\n};"
        "return import(module:'Matte.Core.Introspect')(value:val__);",
        source
    );
    matteValue_t v = matte_run_source(m, matte_string_get_c_str(src));
    matteVM_t * vm = matte_get_vm(m);
    matte_string_destroy(src);
    if (v.binID == 0)
        return matte_string_get_c_str(matte_js_run__stdout);

    matte_string_concat_printf(
        matte_js_run__stdout,
        "%s\n", 
            matte_string_get_c_str(matte_value_string_get_string_unsafe(matte_vm_get_store(vm), matte_value_as_string(matte_vm_get_store(vm), v)))
    );
    matte_store_recycle(matte_vm_get_store(vm), v);
    matte_destroy(m);
    return matte_string_get_c_str(matte_js_run__stdout);
}
