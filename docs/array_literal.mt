@:a = ['a', 'b', 'c', 'd'];

// should print c
print(:a[2]);

@:b = ['z', a, 10];

// should print 10
print(:b[b->size-1]);


@:c = [];
// should print 0
print(:c->size);

