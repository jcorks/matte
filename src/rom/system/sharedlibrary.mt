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
@:class = import(module:'Matte.Core.Class');

@:_sharedlibrary_open = getExternalFunction(name:"_sharedlibrary_open");
@:_sharedlibrary_getCallable = getExternalFunction(name:"_sharedlibrary_getSymbol");
@:_sharedlibrary_callCallable = getExternalFunction(name:"_sharedlibrary_getSymbol");


@TYPES = {
    VOID : 0
    INT : 1,
    DOUBLE : 2,
    FLOAT : 3,
    CHAR : 4,
    CSTR : 5,
    POINTER : 6,
    UINT : 7,
    INT8_T : 8,
    INT16_T : 9
    INT32_T : 10,
    INT64_T : 11
    
}

@SharedLibrary = class(
    name: 'Matte.System.MemoryBuffer',
    statics : {
        TYPES : {get::<-TYPES}
    },
    define:::(this) {
        @handle;
        @:callables = {}

        this.constructor = ::(path) {
            when(handle) empty;
            handle = _sharedlibrary_open(a:path);
        };
    

        
        this.interface = {

        
            bind ::(
                name => String, 
                argTypes => Object, 
                returnType => Number
            ) {
                // should throw error if invalid
                @symbol = _sharedlibrary_getCallable(a:handle, b:name);
                callables[name] = {
                    symbol : symbol,
                    args : argTypes,
                    returnType : returnType
                }
            },
            
            
            call ::(
                args => Object
            ) {
                
            }
        }
    }
});

/*

@:lib = SharedLibrary.new(path:'libtest.so');
@:lib_print = lib.bind(
    symbol:'print', 
    argTypes: [SharedLibrary.TYPES.CSTR, SharedLibrary.TYPES.CSTR], 
    returnType:SharedLibrary.TYPES.VOID
);

@:name = 'Audrey'
lib_print.call(args:['Hello, %s', name]);
*/


return SharedLibrary;
