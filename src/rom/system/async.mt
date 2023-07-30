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
// Workers are a generalization of asynchronous behavior
//
// Workers can be implemented with threads, interprocess communication (IPC), or 
// even separate machines over the network
//
// Each runtime instance (aka VM) can refer to any number of 
// children and may refer to a parent. 


// arg0: path to source to load 
// arg1: input data string
// returns: worker id
@_workerstart = getExternalFunction(name:"__matte_::async_start");


// arg0: worker id
// returns: state number 
// 0 -> running 
// 1 -> finish successs
// 2 -> finish fail
// 3 -> unknown
@_workerstate = getExternalFunction(name:"__matte_::async_state");


// arg0: worker id
// always a string.
@_workerresult = getExternalFunction(name:"__matte_::async_result");

// Send a message to a referrable worker.
// arg0: worker id if a child. if empty, sending message to the parent
// arg1: string message.
@_workersendmessage = getExternalFunction(name:"__matte_::async_sendmessage");


// returns a string if a pending message
// 0-> worker id. always populated
// 1-> message (string)
@_workernextmessage = getExternalFunction(name:"__matte_::async_nextmessage");

// returns a string IFF the worker ended in error. Else, empty
// arg0: worker id. 
@_workererror = getExternalFunction(name:"__matte_::async_error");


@EventSystem = import(module:'Matte.Core.EventSystem');
@Time        = import(module:'Matte.System.Time');
@class       = import(module:'Matte.Core.Class');


@workers = ::<={
    @:o = {};
    @:push = ::(value) {
        o[o.length] = value;
    };
    o->setAttributes(
        attributes : {
            '.' : {
                get ::(key) {
                    when(key == 'length') o->keycount;
                    when(key == 'push') push;
                    error(detail:'this is an array.. what u doing?? (internal async error)');
                }
            }
        }   
    );
    return o;
};
return class(
    name : 'Matte.System.Async',
    inherits:[EventSystem],
    define:::(this) {

        this.events = {
            onNewParentMessage::{}
        };

        @idToWorker::(id) {
            return [::]{
                for(0 : workers.length)::(i){
                    if (workers[i].id == id) send(message:workers[i]);
                };

                // is parent
            };
        };

        // message checking
        @checkMessages::{
            [::]{
                forever(do:::{
                    @nextm = _workernextmessage();
                    when(nextm != empty) ::<={
                        idToWorker(id:nextm[0]).emit(
                            event:'onNewMessage',
                            detail:  nextm[1]                        
                        );
                    };
                    send();
                });
            };
        };
        
        // state checking 
        @checkStates::{
            workers->foreach(do:::(k, v) {
                v.updateState();
            });
        };

        @asinstance = this;
        @Worker = class(
            name : 'Matte.System.Async.Worker',
            inherits : [EventSystem],
            statics : {
                State : {
                    Running  : 0,
                    Finished : 1,
                    Failed   : 2,
                    Unknown  : 3
                }    
            },
            define:::(this) {
                @:State = this.class.State;
                @id;
                @curstate = State.Unknown;
                this.events = {
                    onNewMessage::(detail){},
                    // when the state is detected to have changed
                    // If the state is Finished, result will return
                    // the result of the worker if present.
                    onStateChange::(detail){}
                };
                
                this.constructor = ::(module, input) {
                    id = _workerstart(a:String(from:module), b:String(from:input));
                    when(id == empty) error(detail:'The worker failed to start with the given args');
                    workers.push(value:this);
                    queryState();

                    return this;
                };

                @queryState ::() => Number {
                    return _workerstate(a:id);
                };

                this.interface = {
                    wait ::{
                        @waiting = true;
                        this.installHook(event:'onStateChange', hook:::(detail){
                            @:state = detail;
                            if (state == State.Finished || state == State.Failed) ::<={
                                waiting = false;
                            };
                        });
                        
                        [::]{
                            forever(do:::{
                                Time.sleep(milliseconds:50);
                                asinstance.update();
                                when(!waiting) send();
                            });
                        };
                        asinstance.update();
                    },

                    // semd ,essage tp a tjread
                    'send'::(message => String){
                        _workersendmessage(a:id, b:message);   
                    },

                    result: {
                        get ::{
                            return _workerresult(a:id);
                        }
                    },
                    
                    id : {
                        get :: {
                            return id;
                        }
                    },

                    updateState:: {
                        @:newState = queryState();
                        if (newState != curstate) ::<= {       
                            this.emit(event:'onStateChange', detail:newState);
                            curstate = newState;
                        };
                    },
                    
                    'error': {
                        get ::{
                            return _workererror(a:id);
                        }
                    }
                };

            }
        );

        this.interface = {
            update::{
                checkMessages();
                checkStates();
            },

            Worker : {get::{return Worker;}}
        };
    }

).new();
