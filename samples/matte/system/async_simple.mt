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
// The async module allows for control over 
// asynchronous instances of Matte. New asynchronous instances that are 
// spawned from another instance are called "workers".
@Async = import(module:'Matte.System.Async');
@JSON  = import(module:'Matte.Core.JSON');


// Creating a new worker requires 2 inputs: 
// First, a module. This is the source that runs within the worker instance.
// The pathing is equivalent to the current instance's import rules.
// Second, a string "input" parameter. This is passed to the worker as a 
// parameter. See async_simple__helper.mt for how it is used.
// Input MUST be a string. If it isnt, it will attempt to be converted.
//
// Once created, worker are immediately started.
@w = Async.Worker.new(
    module : 'async_simple__helper.mt',
    input  : JSON.encode(object:{a:"hello", count:4200})
);

w.installHooks(events:{
    // Workers are eventsystems and have a set of events 
    // that are hookable. onStateChange is one of the most useful.
    'onStateChange' :::(detail) {
        // When successful, the state will change to 'Finished'.
        when(detail == Async.Worker.State.Finished) ::<= {
            print(message:'Done. Computed result: ' + w.result);    
        }
        
        // When an error occurs in the worker, the state will change to 'Failed'.
        when(detail == Async.Worker.State.Failed) ::<= {
            print(message:'Failed');    
            print(message:'From worker:');
            print(message:w.error);
        }
    },

    // Communication can occur between the parent and the workers
    // Listening for messages is done with an event hook.
    'onNewMessage' :::(detail) {
            print(message:'Message from worker: ' + detail);
        
    }
});

// Similarly, we can send messages to the child.
w.send(message:'Hi!');


// To wait for workers, a convenience function "wait()"
// can be called. It will only return once the worker is finished.
// As it waits, it internally calls "Async.update()" to 
// continuing gathering events. This means that while waiting,
// all the eventsystem hook events will be listened for and may 
// be called accordingly.
w.wait();

