@MatteString = import(module:"Matte.Core.String");
@class = import(module:"Matte.Core.Class");


@_nextRand = getExternalFunction(name:"__matte_::utility_nextRand");
@_run = getExternalFunction(name:"__matte_::utility_system");
getExternalFunction(name:"__matte_::utility_initRand")();


return class(definition:{
    name : 'Matte.Core.Utility',
    instantiate::(this) {
        this.interface = {
            
            // returns a random value between 0 and 1.
            random : {
                get ::{
                    return _nextRand();
                }
            },


            /// time 
            exit : getExternalFunction(name:"__matte_::utility_exit"),


            /// Execute command in the environment shell
            run ::(command) {
                _run(a:command);
            }
        };
    }
}).new();
