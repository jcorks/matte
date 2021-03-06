// Filesystem allows interacting with the running filesystem.
// This grants the abilities to: look for files, change working directories,
// read / write both string and binary files.
// Binary files are worked with as MemoryBuffers, so check out 
// the memorybuffer_simple.mt to see how they work if more info is 
// needed.
@:Filesystem = import(module:'Matte.System.Filesystem');
@:MBuffer = import(module:'Matte.System.MemoryBuffer');



// The Filesystem current working directory (cwd) always starts 
// within the path of the file that first includes it. 
// The cwd property is editable.
print(message: 'Currently working within: '+Filesystem.cwd);


// directoryContents is a read-only array that shows 
// the contents of the cwd. Each element has a name, 
// whether its a file, and its fully qualified path.
print(message: 'Current working contents:');
foreach(in:Filesystem.directoryContents, do:::(index, file) {
    print(message: 'name: ' + file.name + ', isFile? ' + file.isFile);
});



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
for(in:[0, 10], do:::(i) {
    writeBuffer[i] = i;
});

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
for(in:[0, readBuffer.size], do:::(i) {
    print(message: 'Byte ' + i + ' = ' + readBuffer[i]);
});


