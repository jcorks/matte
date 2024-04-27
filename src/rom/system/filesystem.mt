/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/
@class = import(:"Matte.Core.Class");
@MemoryBuffer = import(:'Matte.Core.MemoryBuffer');

@_getcwd = getExternalFunction(:"__matte_::filesystem_getcwd");
@_setcwd = getExternalFunction(:"__matte_::filesystem_setcwd");
@_directoryEnumerate = getExternalFunction(:"__matte_::filesystem_directoryenumerate");
@_directoryObjectCount  = getExternalFunction(:"__matte_::filesystem_directoryobjectcount");
@_directoryObjectName   = getExternalFunction(:"__matte_::filesystem_directoryobjectname");
@_directoryObjectPath   = getExternalFunction(:"__matte_::filesystem_directoryobjectpath");
@_directoryObjectIsFile = getExternalFunction(:"__matte_::filesystem_directoryobjectisfile");
@_readBytes = getExternalFunction(:"__matte_::filesystem_readbytes");

@_readString = getExternalFunction(:"__matte_::filesystem_readstring");
@_writeString = getExternalFunction(:"__matte_::filesystem_writestring");
@_writeBytes = getExternalFunction(:"__matte_::filesystem_writebytes");
@_remove = getExternalFunction(:"__matte_::filesystem_remove");
@_getFullPath = getExternalFunction(:"__matte_::filesystem_getfullpath");
@_exists = getExternalFunction(:"__matte_::filesystem_exists");

return class(
    name : 'Matte.System.Filesystem',
    define:::(this) {
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
            cwdUp : getExternalFunction(:"__matte_::filesystem_cwdup"),
            
            // returns a Matte.Array describing the contents of the current directory.
            // Each member of the array is an object with the following 
            directoryContents : {
                get :: {
                    _directoryEnumerate();
                    @:ct = _directoryObjectCount();
                    @:files = [];
                    for(0, ct)::(i){
                        files[i] = {
                            name : _directoryObjectName(a:i),
                            path : _directoryObjectPath(a:i), // full path
                            isFile : _directoryObjectIsFile(a:i)
                        }
                    }
                    return files;
                }
            },
            
            // Gets the full path of the given partial or full path
            // from the cwd. 
            // If no such path exists, empty is returned.
            getFullPath ::(path) {
                return _getFullPath(a:path);
            },

            ///// file IO

            // reads the contents of a file and returns a string of its contents
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readString ::(path) {
                return _readString(a:path);
            } ,

            // reads the contents of a file and returns MemoryBuffer of its contents.
            // Expects one argument: a path to the file
            // On failure, throws an error.
            readBytes : ::(path){            
                @:m = MemoryBuffer.new()  
                m.bindNative(:_readBytes(a:path));
                return m;
            },

            // Given a path and a string, writes the given file.
            // on failure, throws an error.
            writeString ::(path, string) {
                _writeString(a:path, b:string);
            },

            // Given a path and a MemoryBuffer, writes the given file.
            // on failure, throws an error.
            writeBytes ::(path, bytes => MemoryBuffer.type) {
                _writeBytes(a:path, b:bytes.handle);
            },
            
            // Attempts to remove the file at the given path.
            remove ::(path) {
                _remove(a:path);
            },
            
            // returns a boolean on whether the given path exists.
            exists ::(path) {
                return _exists(a:path);
            }
        
        }
        
    }

).new();
