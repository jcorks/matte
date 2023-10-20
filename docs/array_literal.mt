@:a = ['a', 'b', 'c', 'd'];

// should print c
print(message:a[2]);

@:b = ['z', a, 10];

// should print 10
print(message:b[b->size-1]);


@:c = [];
// should print 0
print(message:c->size);

