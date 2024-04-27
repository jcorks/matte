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
    Simple example that demonstrates adding a custom 
    module.

*/

#include <matte.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    // create a matte instance
    matte_t * m = matte_create();
    
    // set defaults for output and events to be 
    // through stdio. If something other than stdio is 
    // required for anything, they can be set here
    matte_set_io(m, NULL, NULL, NULL);
    
    
    // Modules are added as bytecode, so any source 
    // we have needs to be compiled into bytecode 
    // first. This process is more commonly done ahead of time,
    // but for demonstration, we compile static source.
    uint32_t bytecodeLength;
    uint8_t * bytecode = matte_compile_source(m, 
        &bytecodeLength,
        // Our module is a simble object with 3 members,
        // a, b, and add. When add is called, it sums 
        // the 2 other members.
        "@:module = {a:0, b:0, add::<- module.a + module.b};" 
        "return module;"
    );
    
    // Now add the module under the name TestModule
    matte_add_module(m, 
        "TestModule",
        bytecode,
        bytecodeLength
    );
    
    // Import the module in code and work with it.
    matteValue_t result = matte_run_source(m,
        "@:m = import(:'TestModule');"
        "m.a = 30;"
        "m.b = 40;"
        "return m.add();"
    );
    
    printf("%s\n", matte_introspect_value(m, result));

    // destroy the instance and cleanup
    matte_destroy(m); 
    return 0;
}
