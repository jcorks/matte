// Contains CLI-only (non-standard) extensions to work 
// with Matte as a shell language.


<@>String = import('Matte.String');
<@>Class  = import('Matte.Class');

<@>System = Class({
    define ::(this, args, classinst) {
        // prints to stdout
        @rawprint = getExternalFunction("system_print");

        this.interface({

            println ::(a) {
                rawprint(a + '\n');
            },

            printf ::(fmt, arr) {
                when (introspect(arr).type() != 'object')::{
                    rawprint(''+fmt);
                }();


                <@>o = String.new(fmt);
                foreach(arr, ::(k, v){
                    <@>key = '$('+k+')';
                    o.replace(key, ''+v);
                });
                rawprint(o);
            },

            getln : getExternalFunction("system_getline")
        });
    }
}).new();


/*
System.println('Hello!');
System.printf('Now presenting variables $(0) and $(1)\n', [20, true]);
System.println("Now tell me, what is your favorite color?");
System.printf("> ");
<@>color = System.getln();
System.printf("Thats an interesting color... hmmm.. $(0)\n", [color]);
*/