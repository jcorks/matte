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



