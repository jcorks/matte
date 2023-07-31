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


static void matte_js_run__print(matteVM_t * vm, const char * str, void * ud) {
    matte_string_concat_printf(
        matte_js_run__stdout,
        "%s\n", str
    );
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
        "return import(module:'Matte.Core.Introspect')(value:(::<={%s}));",
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
