@MatteString = import("Matte.Core.String");
@class = import("Matte.Core.Class");


@_nextRand = getExternalFunction("__matte_::utility_nextRand");

getExternalFunction("__matte_::utility_initRand")();


return class({
    name : 'Matte.Core.Utility',
    define::(this) {
        this.interface({
            
            // returns a random value between 0 and 1.
            random : {
                get ::{
                    return _nextRand();
                }
            },


            /// time 
            exit : getExternalFunction("__matte_::utility_exit"),


            /// Execute command in the environment shell
            run : getExternalFunction("__matte_::utility_system")
        });
    }
}).new();
