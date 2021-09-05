@String = import("Matte.String");
@Class = import("Matte.Class");
@Array = import("Matte.Array");

// Contains CLI-only (non-standard) extensions to work 
// with Matte as a shell language.
return class({
    define::(this, args, classinst) {
        @_print = getExternalFunction("system_print");

        @_getcwd = getExternalFunction("system_getcwd");
        @_setcwd = getExternalFunction("system_setcwd");

        @_directoryEnumerate = getExternalFunction("system_directoryEnumerate");
        @_directoryObjectCount  = getExternalFunction("system_directoryObjectCount");
        @_directoryObjectName   = getExternalFunction("system_directoryObjectName");
        @_directoryObjectPath   = getExternalFunction("system_directoryObjectPath");
        @_directoryObjectIsFile = getExternalFunction("system_directoryObjectIsFile");


        @_readString = getExternalFunction("system_readString");


        @files = Array.new();
        
        
        this.interface({
            ////// IO
            
            println ::(a) {
                _print(a + '\n');
            },

            printf ::(fmt, arr) {
                when (introspect(arr).type() != 'object')::{
                    _print(''+fmt);
                }();


                <@>o = String.new(fmt);
                foreach(arr, ::(k, v){
                    <@>key = '$('+k+')';
                    o.replace(key, ''+v);
                });
                _print(o);
            },

            getln : getExternalFunction("system_getline"),



            // Directory / Files 
            cwd : {
                get :: {
                    return _getcwd();
                },
                
                set ::(v){
                    _setcwd(v);                
                }
            },
            
            // changes directory to the directory above.
            // Throws an error on failure.
            cwdUp :: getExternalFunction("system_cwdUp");
            
            // returns a Matte.Array describing the contents of the current directory.
            // Each member of the array is an object with the following 
            directoryContents : {
                get :: {
                    _directoryEnumerate();
                    <@>ct = _directoryObjectCount();
                    <@>files = Array.new();
                    files.setSize(ct);
                    for([0, ct], ::(i){
                        files[i] = {
                            name : _directoryObjectName(i);
                            path : _directoryObjectPath(i);
                            isFile : _directoryObjectIsFile(i)
                        }
                    }
                    return files;
                }
            },
            
            
            //
            readString : getExternalFunction("system_readString"),
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
