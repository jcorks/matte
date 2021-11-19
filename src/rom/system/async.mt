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
@_workerstart = getExternalFunction("__matte_::async_start");


// arg0: worker id
// returns: state number 
// 0 -> running 
// 1 -> finish successs
// 2 -> finish fail
// 3 -> unknown
@_workerstate = getExternalFunction("__matte_::async_state");

// Gets the input that a parent passed to thsi worker.
// returns: empty if no parent, else a string.
@_workerinput = getExternalFunction("__matte_::async_input");

// arg0: worker id
// always a string.
@_workerresult = getExternalFunction("__matte_::async_result");

// Send a message to an referrable worker.
// arg0: worker id if a child. if empty, sending message to the parent
// arg1: string message.
@_workersendmessage = getExternalFunction("__matte_::async_sendmessage");


// returns a string if a pending message
// 0-> worker id. number if child, empty if parent
// 1-> message (string)
@_workernextmessage = getExternalFunction("__matte_::async_nextmessage");

// returns a string IFF the worker ended in error. Else, empty
// arg0: worker id. 
@_workererror = getExternalFunction("__matte_::async_error");


@Array       = import('Matte.Core.Array');
@EventSystem = import('Matte.Core.EventSystem');
@Time        = import('Matte.System.Time');
@class       = import('Matte.Core.Class');


@workers = Array.new();
@Async;
return class({
    name : 'Matte.System.Async',
    inherits:[EventSystem],
    define::(this) {

        this.events = {
            onNewParentMessage::{}
        };

        @idToWorker::(id) {
            return listen(::{
                for([0, workers.length], ::(i){
                    if (workers[i].id == id) send(workers[i]);
                });

                // is parent
                return empty;
            });
        };

        // message checking
        @checkMessages::{
            loop(::{
                @nextm = _workernextmessage();
                when(nextm != empty) ::<={
                    // empty means from parent
                    if (nextm[0] == empty) ::<= {
                        this.emitEvent(
                            'onNewParentMessage',
                            {
                                message : nextm[1]
                            }
                        );
                    } else ::<= {                    
                        idToWorker(nextm[0]).emitEvent(
                            'onNewMessage',
                            {
                                message : nextm[1]
                            }
                        );
                    };
                    return true;
                };
                return false;
            });
        };
        
        // state checking 
        @checkStates::{
            foreach(workers, ::(k, v) {
                v.updateState();
            });
        };


        @Worker = class({
            name : 'Matte.System.Async.Worker',
            inherits : [EventSystem],
            define::(this, args, classinst) {
                <@>State = classinst.State;
                @id = _workerstart(String(args.importPath), String(args.input));
                @curstate = State.Unknown;
                when(id == empty) error('The worker failed to start with the given args');
                workers.push(this);
                this.events = {
                    onNewMessage::{},
                    // when the state is detected to have changed
                    // If the state is Finished, result will return
                    // the result of the worker if present.
                    onStateChange::{}
                };
                

                @queryState ::() => Number {
                    return _workerstate(id);
                };
                queryState();

                this.interface({
                    wait ::{
                        @waiting = true;
                        this.installHook('onStateChange', ::(worker, state){
                            if (state == State.Finished || state == State.Failed) ::<={
                                waiting = false;
                            };
                        });
                        
                        loop(::{
                            Time.sleep(50);
                            Async.update();
                            return waiting;
                        });
                    },

                    // semd ,essage tp a tjread
                    sendMessage ::(m => String){
                        _workersendmessage(id, m);   
                    },

                    result: {
                        get ::{
                            return _workerresult(id);
                        }
                    },

                    updateState:: {
                        <@>newState = queryState();
                        if (newState != curstate) ::<= {       
                            this.emitEvent('onStateChange', newState);
                            curstate = newState;
                        };
                    },
                    
                    'error': {
                        get ::{
                            return _workererror(id);
                        }
                    }
                });

            }
        });
        Worker.State = {
            Running  : 0,
            Finished : 1,
            Failed   : 2,
            Unknown  : 3
        };

        this.interface({
            update::{
                checkMessages();
                checkStates();
            },

            Worker : {get::{return Worker;}},

            input : {
                get::{return _workerinput();}
            },

            sendToParent::(m) {
                _workersendmessage(empty, m);
            }
        });
    }

}).new();



/*
@Async = import('Matte.System.Async');
@JSON = import('Matte.Core.JSON');



@w = Async.Worker.new({
    importPath: 'helper.mt',
    input : JSON.encode({a:"hello", count:4200})
});



loop(::<={        
    @isDone = false;
    w.installHook('onStateChange', ::(worker, state) {
        when(state == Async.Worker.State.Finished) ::<= {
            print('Done');    
            isDone = true;
        };
        
        when(state == Async.Worker.State.Failed) ::<= {
            print('Failed');    
            print('From worker:');
            print(w.error);
            isDone = true;
        };
    });

    w.installHook('onNewMessage', ::(worker, message) {
        print('Message from worker: ' + message);
    });

    
    return ::{
        for([0, 10], ::(i) {
            print(introspect.arrayToString([97+i]));
        });
        Async.update();
        return !isDone;
    };
});






*/

