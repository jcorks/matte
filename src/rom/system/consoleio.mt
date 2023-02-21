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
@class = import(module:"Matte.Core.Class");
@_print = getExternalFunction(name:"__matte_::consoleio_print");

return class(
    name : 'Matte.System.ConsoleIO',
    define:::(this) {
        this.interface = {
            // Prints a simple string with a newline afterwards.
            println ::(message => String) {
                _print(a:message + '\n');
            },
            
            put ::(string => String) {
                _print(a:string);
            },


            // Prints a formatted string
            // The first argument must be a string.
            printf ::(format => String, args) {
                when (args->type != Object)::<={
                    _print(a:''+format);
                };


                @o = format;
                args->foreach(do:::(k, v){
                    @:key = '$('+k+')';
                    o = o->replace(key:key, with:''+v);
                });
                _print(a:o);
            },

            getln : getExternalFunction(name:"__matte_::consoleio_getline"),

            clear : getExternalFunction(name:"__matte_::consoleio_clear")
        };
    }
).new();
