@class = import(module:"Matte.Core.Class");
@_print = getExternalFunction(name:"__matte_::consoleio_print");

return class(
    name : 'Matte.System.ConsoleIO',
    define:::(this) {
        this.interface = {
            // Prints a simple string with a newline afterwards.
            println ::(message => String) {
                _print(a:message + '\n');
            },
            
            put ::(string => String) {
                _print(a:string);
            },


            // Prints a formatted string
            // The first argument must be a string.
            printf ::(format => String, args) {
                when (args->type != Object)::<={
                    _print(a:''+format);
                };


                @o = format;
                args->foreach(do:::(k, v){
                    @:key = '$('+k+')';
                    o = o->replace(key:key, with:''+v);
                });
                _print(a:o);
            },

            getln : getExternalFunction(name:"__matte_::consoleio_getline"),

            clear : getExternalFunction(name:"__matte_::consoleio_clear")
        };
    }
).new();
