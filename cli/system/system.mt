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



        @_sleepms = getExternalFunction("system_sleepms");

        
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
            




            /// random
            
            // returns a random value between 0 and 1.
            random : {
                get ::{
                    return _nextRand();
                }
            },


            /// time 


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
