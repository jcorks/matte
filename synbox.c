#include <emscripten.h>
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
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