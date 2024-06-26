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
    Simple example that demonstrates importing 
    modules as files.

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
    
    // By default, the Matte import() function only 
    // looks for built-in core and system modules or 
    // modules that have been added using matte_add_module.
    // But, it is possible to set a custom importer 
    // that extends this behavior. If passed NULL, a 
    // default file-based importer is setup.
    matte_set_importer(m, NULL, NULL);
    
    
    // matte_run_source is a quick way to run raw 
    // source. The IO output function is used if an 
    // issue is encountered.
    matteValue_t result = matte_run_source(m, 
        // import 2 modules, each returning an object 
        // with a member called "myFunction"
        "@:m1 = import(:'module1.mt');"
        "@:m2 = import(:'module2.mt');"
        ""
        // return the result of adding them
        "return m1.myFunction() + m2.myFunction();"
    );
    
    
    printf("%s\n", matte_introspect_value(m, result));

    // destroy the instance and cleanup
    matte_destroy(m); 
    return 0;
}
