@class = import("Matte.Class");

@_getcwd = getExternalFunction("system_getcwd");
@_setcwd = getExternalFunction("system_setcwd");

@_directoryEnumerate = getExternalFunction("system_directoryenumerate");
@_directoryObjectCount  = getExternalFunction("system_directoryobjectcount");
@_directoryObjectName   = getExternalFunction("system_directoryobjectname");
@_directoryObjectPath   = getExternalFunction("system_directoryobjectpath");
@_directoryObjectIsFile = getExternalFunction("system_directoryobjectisfile");


return class({
    define(this) {
        this.interface({
            // Directory / Files 
            cwd : {
                get :: {
                    return _getcwd();
                },
                
                set ::(v => String){
                    _setcwd(v);                
                }
            },
            
            // changes directory to the directory above.
            // Throws an error on failure.
            cwdUp : getExternalFunction("system_cwdup"),
            
            // returns a Matte.Array describing the contents of the current directory.
            // Each member of the array is an object with the following 
            directoryContents : {
                get :: {
                    _directoryEnumerate();
                    <@>ct = _directoryObjectCount();
                    <@>files = Array.new();
                    files.length = ct;
                    for([0, ct], ::(i){
                        files[i] = {
                            name : _directoryObjectName(i),
                            path : _directoryObjectPath(i), // full path
                            isFile : _directoryObjectIsFile(i)
                        };
                    });
                    return files;
                }
            },
            
            

            ///// file IO

            // reads the contents of a file and returns a string of its contents
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readString : getExternalFunction("system_readstring"),

            // reads the contents of a file and returns an array of byte values of its contents.
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readBytes : getExternalFunction("system_readbytes"),


            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeString : getExternalFunction("system_writestring"),

            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeBytes : getExternalFunction("system_writebytes"),        
        
        });
    }

}).new();
