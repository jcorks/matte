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
/*
    Example that transforms source into portable bytecode.

*/

#include <matte.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be set here
    matte_set_io(m, NULL, NULL, NULL);
    
    
    // Compiles the source into bytecode instead of 
    // running it right away. This is a way to package 
    // Matte code. The bytecode could theoretically be used in a separate 
    // VM and could be run with equivalent results
    uint32_t bytecodeSize;
    uint8_t * bytecode = matte_compile_source(m, 
        &bytecodeSize,
        "@:add ::(a, b) {"
        "  return a + b;"
        "};"
        ""
        "return add(a:'My favorite number is ', b:256);"
    );
    
    printf("The source converted to bytecode was %d bytes\n", bytecodeSize);
    
    
    // Now run the bytecode.
    matteValue_t result = matte_run_bytecode(m, bytecode, bytecodeSize);

    // free the bytecode
    free(bytecode);

    printf("%s\n", matte_introspect_value(m, result));

    // destroy the instance and cleanup
    matte_destroy(m); 

    return 0;
}
