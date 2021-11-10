@MatteString = import("Matte.String");
@class = import("Matte.Class");


@_sleepms = getExternalFunction("system_sleepms");
@_getTicks = getExternalFunction("system_getticks");
@initTime = _getTicks();

@tasks = Array.new();

return class({
    define(this) {
        this.interface({

            // sleeps approximately the given number of milliseconds
            sleep ::(s => Number){return _sleepms(s*1000);},

            // Gets the current time, as from the C standard time() function with the 
            // NULL argument.
            time : getExternalFunction("system_time"),

            // Gets the number of milliseconds since the instance of 
            getTicks ::{
                return _getTicks() - initTime;
            },


            /// tasks

            // adds a new task. The return value of fn determines whether to keep 
            // the task active or not. msTimeout is optional, denoting a minimum 
            // wait time before initiating the same task again.
            addTask ::(fn, msTimeout) {
                tasks.push({
                    fn: fn, 
                    timeout: if(msTimeout == empty) 0 else msTimeout,
                    last: this.getTicks()
                });
            },


            // runs all pending tasks. Only returns once all tasks are done.
            doTasks ::{
                loop(::{
                    when(tasks.length == 0) false;
                    for([0, tasks.length], ::(i){
                        <@>task = tasks[i];
                        when(task == empty) tasks.length;


                        when(this.getTicks() > task.last + task.timeout) ::<={
                            task.last = this.getTicks();
                            when(!task.fn()) ::<={
                                tasks.remove(i);
                                return i-1;
                            };
                        };
                    });


                    // sleep for a "safe" period based on the least 
                    // amount needed to wait for next task
                    @leastWait = -1;
                    @timenow = this.getTicks();
                    for([0, tasks.length], ::(i){
                        <@>task = tasks[i];
                        @timeNeeded = (task.last + task.timeout) - this.getTicks();
                        if (((leastWait < 0) || (timeNeeded < leastWait)) && timeNeeded > 0) ::<={
                            leastWait = timeNeeded;
                        };
                    });


                    if (leastWait > 0 && leastWait > 100) ::<={
                        _sleepms(leastWait * 0.8);
                    };
                    return true;
                });
            },
        });
    }
}).new();
