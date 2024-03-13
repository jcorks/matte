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
    @already = {}
    @strings = [];
    @pspace ::(level) {
        for(0,level)::{
            strings->push(value:'  ');
        }
    }
    @helper ::(obj, level) {
        @poself = helper;

        match(obj->type) {
            (String) :  strings->push(value:'(type => String): \'' + obj + '\''),
            (Number) :  strings->push(value:'(type => Number): '+obj),
            (Boolean):  strings->push(value:'(type => Boolean): '+obj),
            (Function): strings->push(value:'(type => Function)'),
            (Empty)  :  strings->push(value:'<empty>'),
            (Type):     strings->push(value:'(type => Type): ' + obj),
            default: ::<={
                when(already[obj] == true) strings->push(value:
                    '(type => '+obj->type+'): [already printed]'
                );
                already[obj] = true;

                strings->push(value:'(type => '+obj->type+'): {');

                @multi = false;
                foreach(obj)::(key, val) {                        
                    strings->push(value:(if (multi) ',\n' else '\n')); 
                    pspace(level:level+1);
                    strings->push(value:String(from:key));
                    strings->push(value:' : ');
                    poself(obj:val, level:level+1);
                    multi = true;
                }
                pspace(level:level);
                if (multi) ::<= {
                    strings->push(value:'\n');
                    pspace(level:level);
                    strings->push(value:'}');
                } else 
                    strings->push(value:'}');
            }
        }
    }
    pspace(level:1);
    helper(obj:value, level:1);
    return String.combine(strings);
}


