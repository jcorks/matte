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
return ::(value, depth) {
    if (depth->type != Number) depth = empty;
    if (depth->type == Number && depth <= 0) depth = empty;
    @already = {}
    @strings = [];
    @pspace ::(level) {
        for(0,level)::{
            strings->push(:'..');
        }
    }
    @helper ::(obj, level) {

        match(obj->type) {
            (String) :  strings->push(:'\'' + obj + '\''),
            (Number) :  strings->push(:''+obj),
            (Boolean):  strings->push(:''+obj),
            (Function): strings->push(:'(Function)'),
            (Empty)  :  strings->push(:''+obj),
            (Type):     strings->push(:'' + obj),
            default: ::<={
                when(depth != empty && level > depth) strings->push(:'[reached depth limit]');

                when(already[obj] == true) strings->push(:
                    '('+obj->type+'): [already printed]'
                );
                already[obj] = true;

                strings->push(:'('+obj->type+'): {');

                @keyLength = 0;
                @keyStr = {};
                foreach(obj)::(key, val) {
                    @:keyS = ::? {
                      return String(:key);
                    } => {
                      onError::(message) {
                        return '('+key->type+')';                       
                      }
                    }
                    if (keyS->length > keyLength)
                      keyLength = keyS->length;
                    keyStr[key] = keyS;
                }
  

                @multi = false;
                foreach(obj)::(key, val) {                        
                    strings->push(:(if (multi) ',\n' else '\n')); 
                    pspace(level:level+1);
                    
                    @keyS = keyStr[key];
                    for(keyS->length, keyLength) ::(i) {
                      keyS = keyS + ' ';
                    }
                    strings->push(:keyS);
                    strings->push(:' : ');
                    helper(obj:val, level:level+1);
                    multi = true;
                }
                if (multi) ::<= {
                    strings->push(:'\n');
                    pspace(level:level);
                    strings->push(:'}');
                } else 
                    strings->push(:'}');
            }
        }
    }
    pspace(level:1);
    helper(obj:value, level:1);
    return String.combine(strings);
}


