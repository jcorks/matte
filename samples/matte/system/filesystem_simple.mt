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
// Filesystem allows interacting with the running filesystem.
// This grants the abilities to: look for files, change working directories,
// read / write both string and binary files.
// Binary files are worked with as MemoryBuffers, so check out 
// the memorybuffer_simple.mt to see how they work if more info is 
// needed.
@:Filesystem = import(module:'Matte.System.Filesystem');
@:MBuffer = import(module:'Matte.Core.MemoryBuffer');



// The Filesystem current working directory (cwd) always starts 
// where the Matte interpreter is started.
print(message: 'Currently working within: '+Filesystem.cwd);


// directoryContents is a read-only array that shows 
// the contents of the cwd. Each element has a name, 
// whether its a file, and its fully qualified path.
print(message: 'Current working contents:');
foreach(Filesystem.directoryContents) ::(index, file) {
    print(message: 'name: ' + file.name + ', isFile? ' + file.isFile);
}



// A file can have its string contents read and dumped.
// Note that readString will throw and error on failure.
@:testPath = Filesystem.directoryContents[0].path;
print(message: 'Reading contents of the first file: ' + testPath);
print(message:
    Filesystem.readString(path:testPath)
);



// Similarly, strings can be written as entire files. If a full path isnt 
// given as the path, relativity from the cwd is assumed.
// (Should throw an error on failure.)
Filesystem.writeString(
    path: 'test.txt',
    string: 'hello, world!'
);



// Alternatively, can work with bytes as memory buffers
@:writeBuffer = MBuffer.new();
writeBuffer.size = 10;
for(0, 10) ::(i) {
    writeBuffer.writeI8(offset:i, value:i);
}

// The byte analogs of writing and reading
// work similarly, except work with MemoryBuffers instead 
// of strings.
Filesystem.writeBytes(
    path: 'test.bin',
    bytes: writeBuffer
);


@:readBuffer = Filesystem.readBytes(
    path:'test.bin'
);


print(message: 'Read byte buffer from test.bin: ' + readBuffer.size + ' bytes');
for(0, readBuffer.size) ::(i) {
    print(message: 'Byte ' + i + ' = ' + readBuffer.readI8(offset:i));
}


