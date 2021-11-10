@MatteString = import("Matte.String");
@class = import("Matte.Class");


@_print = getExternalFunction("system_print");

return class({
    define(this) {
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

            getln : getExternalFunction("system_getline"),

            clear : getExternalFunction("system_clear")
        });
    }
}).new();
