@String = import("Matte.String");
// Contains CLI-only (non-standard) extensions to work 
// with Matte as a shell language.
return {
    // prints to stdout
    print : getExternalFunction("system_print"),
    printf ::(fmt, arr) {
        <@> str = String.new(fmt);
        <@> spl = str.split('$$');
        @ strout = String.new();

        foreach(spl, ::(item){
            
        });
    },
    getln : getExternalFunction("system_getline")
};