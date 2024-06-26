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
// Only for children workers.
// Allows for communication with the parent
@:AsyncWorker = import(:'Matte.System.AsyncWorker');
@:JSON = import(:'Matte.Core.JSON');



// Async parameters are always within parameters.input as a string.
@input = JSON.decode(:parameters.input);


AsyncWorker.sendToParent(
    :'Im alive! you sent me input:' + parameters.input
);

AsyncWorker.installHook(
    event:'onNewMessage', 
    hook:::(detail) {
        print(:'Message from parent: ' + detail);
    }
);

@out = 0;
for(0, input.count) ::(i) {
    out += i;
}
AsyncWorker.update(); // checks for new messages


// workers MUST produce string results, else the parent 
// won't be able to interpret the result.
return ''+out;


