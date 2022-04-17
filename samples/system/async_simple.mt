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

w.installHooks(events:[
    // Workers are eventsystems and have a set of events 
    // that are hookable. onStateChange is one of the most useful.
    {
        event:'onStateChange', 
        hook::(detail) {
        // When successful, the state will change to 'Finished'.
        when(detail == Async.Worker.State.Finished) ::<= {
            print(message:'Done. Computed result: ' + w.result);    
        };
        
        // When an error occurs in the worker, the state will change to 'Failed'.
        when(detail == Async.Worker.State.Failed) ::<= {
            print(message:'Failed');    
            print(message:'From worker:');
            print(message:w.error);
        };
    },

// Communication can occur between the parent and the workers
// Listening for messages is done with an event hook.
    {
        event:'onNewMessage', 
        hook::(detail) {
            print(message:'Message from worker: ' + detail);
        }
    }
]);

// Similarly, we can send messages to the child.
w.send(message:'Hi!');


// To wait for workers, a convenience function "wait()"
// can be called. It will only return once the worker is finished.
// As it waits, it internally calls "Async.update()" to 
// continuing gathering events. This means that while waiting,
// all the eventsystem hook events will be listened for and may 
// be called accordingly.
w.wait();

