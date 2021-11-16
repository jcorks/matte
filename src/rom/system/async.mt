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

// Wait for worker to exit.
// arg0: worker id
@_workerjoin = getExternalFunction("__matte_::async_join");


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




@Array       = import('Matte.Core.Array');
@EventSystem = import('Matte.Core.EventSystem');


@workers = Array.new();
@ASync = class({
    name : 'Matte.System.Async',
    inherits:[EventSystem],
    define::(this) {

        this.events = {
            onNewMessage::{}
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
                    this.emitEvent({
                        'onNewMessage'
                        {
                            id: idToWorker(nextm[0]),
                            message : nextm[1]
                        }
                    });
                    return true;
                };
                return false;
            });
        };


        @Worker = class({
            name : 'Matte.System.Async.Worker',
            define::(this, args, classinst) {
                @id = _workerstart(args.sourcepath, args.input);
                @curstate = State.Unknown;
                when(!id) error('The worker failed to start with the given args');
                workers.push(this);

                @queryState ::() => State.type {
                    curstate = match(_workerstate(id)) {
                        (0): State.Running,
                        (1): State.Finished,
                        (2): State.Failed,
                        (3): State.Unknown
                    };
                };
                queryState();

                this.interface({
                    join ::{
                        _workerjoin(id);
                    },

                    // semd ,essage tp a tjread
                    sendMessage ::(m => String){
                        _workersendmessage(id, m);   
                    },

                    state: {
                        get ::{
                            queryState();
                            return curstate;
                        }
                    },


                    output : {
                        return _workerresult(id);
                    }
                });

            }
        });
        Worker.State = enum({
            Running  : 0,
            Finished : 1,
            Failed   : 2,
            Unknown  : 3
        }, 'Matte.System.Async.Worker.State');


        this.interface({
            update::{
                checkMessages();
            },

            Worker : {get::{return Worker;}},

            sendToParent::(m) {
                _workersendmessage(empty, m);
            }
        });
    }

}).new();


return ASync;