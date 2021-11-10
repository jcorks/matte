@MatteString = import("Matte.String");
@class = import("Matte.Class");


@_print = getExternalFunction("system_print");
@_nextRand = getExternalFunction("system_nextRand");

getExternalFunction("system_initRand")();


return class({
    define(this) {
        this.interface({
            
            // returns a random value between 0 and 1.
            random : {
                get ::{
                    return _nextRand();
                }
            },


            /// time 
            exit : getExternalFunction("system_exit"),


            /// Execute command in the environment shell
            run : getExternalFunction("system_system")
        });
    }
}).new();
