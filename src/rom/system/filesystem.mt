@class = import("Matte.Core.Class");
@Array = import('Matte.Core.Array');
@MemoryBuffer = import('Matte.System.MemoryBuffer');

@_getcwd = getExternalFunction("__matte_::filesystem_getcwd");
@_setcwd = getExternalFunction("__matte_::filesystem_setcwd");

@_directoryEnumerate = getExternalFunction("__matte_::filesystem_directoryenumerate");
@_directoryObjectCount  = getExternalFunction("__matte_::filesystem_directoryobjectcount");
@_directoryObjectName   = getExternalFunction("__matte_::filesystem_directoryobjectname");
@_directoryObjectPath   = getExternalFunction("__matte_::filesystem_directoryobjectpath");
@_directoryObjectIsFile = getExternalFunction("__matte_::filesystem_directoryobjectisfile");
@_readBytes = getExternalFunction("__matte_::filesystem_readbytes");
return class({
    name : 'Matte.System.Filesystem',
    define::(this) {
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
            cwdUp : getExternalFunction("__matte_::filesystem_cwdup"),
            
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
            readString : getExternalFunction("__matte_::filesystem_readstring"),

            // reads the contents of a file and returns MemoryBuffer of its contents.
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readBytes : ::(p){
            
                return MemoryBuffer.new(_readBytes(p));  
            },

            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeString : getExternalFunction("__matte_::filesystem_writestring"),

            // Given a path and a MemoryBuffer, writes the given file.
            // on failure, throws an error.
            writeBytes : getExternalFunction("__matte_::filesystem_writebytes")   
        
        });
    }

}).new();
