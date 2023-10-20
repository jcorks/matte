@:a = ['d', 'e', 'f'];

// Becomes a shallow copy of a
@:b = [...a];


// c makes 'a' 'b' 'c' 'd' 'e' 'f' 'g'
@:c = [
    'a',
    'b',
    'c',
    ...a,
    'g'
];





@d = {
    "test" : "test2",
    ...a
}


// Should print e
print(message: d[1]);



