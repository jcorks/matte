@_threadstart = getExternalFunction("__matte_::thread_start");
@_threadjoin = getExternalFunction("__matte_::thread_join");
// 0 -> running 
// 1 -> finish successs
// 2 -> finish fail
// 3 -> unknown
@_threadstate = getExternalFunction("__matte_::thread_state");

// always a string.
@_threadresult = getExternalFunction("__matte_::thread_result");

// returns an array, 
// 0-> thread id,
// 1-> message (string)
@_threadnextmessage = getExternalFunction("__matte_::thread_nextmessage");



@State = enum({
    Running  : 0,
    Finished : 1,
    Failed   : 2,
    Unknown  : 3
}, 'Thread.State');





@Thread = Class({
    name : 'System.Thread',
    define::(this, args, classinst) {
        @id = 0;

        @initialize
        @id = _threadstart(args.sourcepath, args.input);
        @curstate = State.Unknown;
        when(!id) error('The thread failed to start with the given args');

        @queryState ::() => State.type {
            curstate = match(_threadstate(id)) {
                (0): State.Running,
                (1): State.Finished,
                (2): State.Failed,
                (3): State.Unknown
            };
        }

        this.interface({
            join ::{
                _threadjoin(id);
            },

            // semd ,essage tp a tjread
            sendMessage ::(m => String){
                _threadsendmessage(id, m);   
            },


            output : {
                return _threadresult(id);
            }
        });

    }
});
Thread.State = State;


// message checking
@messages = Array.new();
@checkMessages ::{
    loop(::{
        @nextm = _threadnextmessage();
        when(nextm != empty) ::<={
            messages.push(
                {
                    id: nextm[0],
                    message : nextm[1]
                }
            );
            return true;
        };
        return false;
    });
};

// Gets any remaining pending message
Thread.nextMessage ::{
    checkMessages();
    when(messages.length) empty;
    return message.pop();
};

return Thread;