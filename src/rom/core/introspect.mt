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
return ::(value) {
    @already = {};
    @pspace ::(level) {
        @str = '';
        [0, level]->for(do:::{
            str = str + '  ';
        });
        return str;
    };
    @helper ::(obj, level) {
        @poself = context;

        return match(obj->type) {
            (String) : '(type => String): \'' + obj + '\'',
            (Number) : '(type => Number): '+obj,
            (Boolean): '(type => Boolean): '+obj,
            (Empty)  : '<empty>',
            (Object) : ::{
                when(already[obj] == true) '(type => Object): [already printed]';
                already[obj] = true;

                @output = '(type => Object): {';

                @multi = false;
                obj->foreach(do:::(key, val) {                        
                    output = output + (if (multi) ',\n' else '\n'); 
                    output = output + pspace(level:level+1)+(String(from:key))+' : '+poself(obj:val, level:level+1);
                    multi = true;
                });
                output = output + pspace(level:level) + (if (multi) '\n' + pspace(level:level)+'}' else '}');
                return output;                
            }(),
            (Type): '(type => Type): ' + obj,
            default: '(type => ' + (obj->type) + '): {...}'

        };
    };
    return pspace(level:1) + helper(obj:value, level:1);
};


