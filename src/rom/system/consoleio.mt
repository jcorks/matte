@MatteString = import("Matte.Core.String");
@class = import("Matte.Core.Class");
@_print = getExternalFunction("__matte_::consoleio_print");

return class({
    name : 'Matte.System.ConsoleIO',
    define::(this) {
        this.interface({
            // Prints a simple string with a newline afterwards.
            println ::(a => String) {
                _print(a + '\n');
            },


            // Prints a formatted string
            // The first argument must be a string.
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

            getln : getExternalFunction("__matte_::consoleio_getline"),

            clear : getExternalFunction("__matte_::consoleio_clear")
        });
    }
}).new();
