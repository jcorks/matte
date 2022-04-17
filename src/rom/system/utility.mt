@:class = import(module:"Matte.Core.Class");


@:_run = getExternalFunction(name:"__matte_::utility_system");
@:_os = getExternalFunction(name:"__matte_::utility_os")();
@:_exit = getExternalFunction(name:"__matte_::utility_exit");
return class(
    name : 'Matte.System.OS',
    define:::(this) {
        this.interface = {
            name : {
                get :: {
                    return _os;
                }
            },
            
            /// time 
            exit ::(code) {
                _exit(a:code);
            } ,


            /// Execute command in the environment shell
            run ::(command) {
                _run(a:command);
            }
        };
    }
).new();
