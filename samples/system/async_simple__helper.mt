// Only for children workers.
// Allows for communication with the parent
@:AsyncWorker = import(module:'Matte.System.AsyncWorker');
@:JSON = import(module:'Matte.Core.JSON');



// Async parameters are always within parameters.input as a string.
@input = JSON.decode(string:parameters.input);


AsyncWorker.sendToParent(
    message:'Im alive! you sent me input:' + parameters.input
);

AsyncWorker.installHook(
    event:'onNewMessage', 
    hook:::(detail) {
        print(message:'Message from parent: ' + detail);
    }
);

@out = 0;
for(in:[0, input.count], do:::(i) {
    out += i;
});
AsyncWorker.update(); // checks for new messages


// workers MUST produce string results, else the parent 
// won't be able to interpret the result.
return ''+out;


