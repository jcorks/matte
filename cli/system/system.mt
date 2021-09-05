@String = import("Matte.String");
@Class = import("Matte.Class");
@Array = import("Matte.Array");

// Contains CLI-only (non-standard) extensions to work 
// with Matte as a shell language.
return class({
    define::(this, args, classinst) {

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
            
            // prints to stdout
            print : getExternalFunction("system_print"),
            
            // prints to stdout use string interpolation
            printf ::(fmt, arr) {
                <@> str = String.new(fmt);
                <@> spl = str.split('$$');
                @ strout = String.new();

                foreach(spl, ::(item){
                    
                });
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
