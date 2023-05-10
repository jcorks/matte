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
@MBuffer = import(module:'Matte.System.MemoryBuffer');


@:a = MBuffer.new();
a.size = 1024;

@:b = MBuffer.new();
b.size = 1024;

a.append(other:b);
print(message: 'New size: ' + a.size);

// Can access bytes directly
a[0] = 255;
b[0] = 255;

// can also remove bytes from a buffer.
// from and to are inclusive indices.
a.remove(from:0, to:1023);
print(message: 'New size after removal: ' + a.size);


// or get a slice of the buffer as a new buffer 
// from and to are inclusive indices.
@:c = a.subset(from:0, to:511);
print(message: 'New subset buffer: ' + c.size);


// or perform a C memset 
c.set(selfOffset:0, value: 23, len:512);
print(message: 'Some values within the c buffer after memset: ' + c[0] + c[1] + c[2]);

// or! perform a C memcpy from some other buffer INTO the acting buffer.
// This will copy 10 bytes from the start of "c" and place it within 
// "a" starting at byte 1:
a.copy(
    selfOffset: 1,
    src: c,
    srcOffset: 0,
    len: 10
);
print(message: 'Some values within the a buffer after memcpy: ' + a[0] + a[1] + a[2]);




// finally when done, buffers are explicitly released:
a.release();
b.release();
c.release();



