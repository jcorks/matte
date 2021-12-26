@class = import(module:"Matte.Core.Class");
@Array = import(module:'Matte.Core.Array');
@MemoryBuffer = import(module:'Matte.System.MemoryBuffer');

@_getcwd = getExternalFunction(name:"__matte_::filesystem_getcwd");
@_setcwd = getExternalFunction(name:"__matte_::filesystem_setcwd");

@_directoryEnumerate = getExternalFunction(name:"__matte_::filesystem_directoryenumerate");
@_directoryObjectCount  = getExternalFunction(name:"__matte_::filesystem_directoryobjectcount");
@_directoryObjectName   = getExternalFunction(name:"__matte_::filesystem_directoryobjectname");
@_directoryObjectPath   = getExternalFunction(name:"__matte_::filesystem_directoryobjectpath");
@_directoryObjectIsFile = getExternalFunction(name:"__matte_::filesystem_directoryobjectisfile");
@_readBytes = getExternalFunction(name:"__matte_::filesystem_readbytes");

@_readString = getExternalFunction(name:"__matte_::filesystem_readstring");
@_writeString = getExternalFunction(name:"__matte_::filesystem_writestring");
@_writeBytes = getExternalFunction(name:"__matte_::filesystem_writebytes");

return class(definition:{
    name : 'Matte.System.Filesystem',
    instantiate::(this) {
        this.interface = {
            // Directory / Files 
            cwd : {
                get :: {
                    return _getcwd();
                },
                
                set ::(value => String){
                    _setcwd(a:value);                
                }
            },
            
            // changes directory to the directory above.
            // Throws an error on failure.
            cwdUp : getExternalFunction(name:"__matte_::filesystem_cwdup"),
            
            // returns a Matte.Array describing the contents of the current directory.
            // Each member of the array is an object with the following 
            directoryContents : {
                get :: {
                    _directoryEnumerate();
                    <@>ct = _directoryObjectCount();
                    <@>files = Array.new();
                    files.length = ct;
                    for(in:[0, ct], do:::(i){
                        files[i] = {
                            name : _directoryObjectName(a:i),
                            path : _directoryObjectPath(a:i), // full path
                            isFile : _directoryObjectIsFile(a:i)
                        };
                    });
                    return files;
                }
            },
            
            

            ///// file IO

            // reads the contents of a file and returns a string of its contents
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readString ::(path) {
                _readString(a:path);
            } ,

            // reads the contents of a file and returns MemoryBuffer of its contents.
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readBytes : ::(p){            
                return MemoryBuffer.new(handle:_readBytes(a:p));  
            },

            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeString ::(path, string) {
                _writeString(a:path, b:string);
            },

            // Given a path and a MemoryBuffer, writes the given file.
            // on failure, throws an error.
            writeBytes ::(path, bytes => MemoryBuffer.type) {
                _writeBytes(a:path, b:bytes);
            }
        
        };
    }

}).new();