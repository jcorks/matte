@MatteString = import("Matte.String");
@Class = import("Matte.Class");
@Array = import("Matte.Array");
@enum  = import("Matte.Enum");




// Contains CLI-only (non-standard) extensions to work 
// with Matte as a shell language.
@System = Class({
    name : "System",
    define::(this, args, classinst) {
        @_getargcount = getExternalFunction("system_getargcount");
        @_getarg = getExternalFunction("system_getarg");
        @_print = getExternalFunction("system_print");

        @_getcwd = getExternalFunction("system_getcwd");
        @_setcwd = getExternalFunction("system_setcwd");

        @_directoryEnumerate = getExternalFunction("system_directoryenumerate");
        @_directoryObjectCount  = getExternalFunction("system_directoryobjectcount");
        @_directoryObjectName   = getExternalFunction("system_directoryobjectname");
        @_directoryObjectPath   = getExternalFunction("system_directoryobjectpath");
        @_directoryObjectIsFile = getExternalFunction("system_directoryobjectisfile");


        @_readString = getExternalFunction("system_readString");
        @_sleepms = getExternalFunction("system_sleepms");
        getExternalFunction("system_initRand")();
        @_nextRand = getExternalFunction("system_nextRand");

        @files = Array.new();
        @tasks = Array.new();
        @_getTicks = getExternalFunction("system_getticks");
        @initTime = _getTicks();
        
        this.interface({
            ////// external args 
            args : {
                get ::{
                    @out = Array.new();
                    @ct = _getargcount();
                    out.length = ct;
                    for([0, ct], ::(i) {
                        out[i] = _getarg(i);
                    });
                    return out;
                }
            }

            ////// IO
            
            println ::(a => String) {
                _print(a + '\n');
            },

            printf ::(fmt => String, arr) {
                when (introspect.type(arr) != Object)::<={
                    _print(''+fmt);
                };


                <@>o = MatteString.new(fmt);
                foreach(arr, ::(k, v){
                    <@>key = '$('+k+')';
                    o.replace(key, ''+v);
                });
                _print(o);
            },

            getln : getExternalFunction("system_getline"),

            clear : getExternalFunction("system_clear"),

            



            // Directory / Files 
            cwd : {
                get :: {
                    return _getcwd();
                },
                
                set ::(v => String){
                    _setcwd(v);                
                }
            },
            
            // changes directory to the directory above.
            // Throws an error on failure.
            cwdUp : getExternalFunction("system_cwdup"),
            
            // returns a Matte.Array describing the contents of the current directory.
            // Each member of the array is an object with the following 
            directoryContents : {
                get :: {
                    _directoryEnumerate();
                    <@>ct = _directoryObjectCount();
                    <@>files = Array.new();
                    files.length = ct;
                    for([0, ct], ::(i){
                        files[i] = {
                            name : _directoryObjectName(i),
                            path : _directoryObjectPath(i), // full path
                            isFile : _directoryObjectIsFile(i)
                        };
                    });
                    return files;
                }
            },
            
            

            ///// file IO

            // reads the contents of a file and returns a string of its contents
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readString : getExternalFunction("system_readstring"),

            // reads the contents of a file and returns an array of byte values of its contents.
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readBytes : getExternalFunction("system_readbytes"),


            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeString : getExternalFunction("system_writestring"),

            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeBytes : getExternalFunction("system_writebytes"),


            /// random
            
            // returns a random value between 0 and 1.
            random : {
                get ::{
                    return _nextRand();
                }
            },


            /// time 

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
                    timeout: if(introspect.type(msTimeout) == Empty) 0 else msTimeout,
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

            exit : getExternalFunction("system_exit")

        });
    }    
}).new();

return System;
/*
System.println('Hello!');
System.printf('Now presenting variables $(0) and $(1)\n', [20, true]);
System.println("Now tell me, what is your favorite color?");
System.printf("> ");
<@>color = System.getln();
System.printf("Thats an interesting color... hmmm.. $(0)\n", [color]);



//@System = import('Matte.System');
context.onError = ::{
    System.println('Error occurred.');
};

@val = 0;
System.addTask(::{
    System.printf('Hi! $(0)\n', [val]);
    val = val+1;
    return true;
}, 40);

System.doTasks();
*/
