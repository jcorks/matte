@:a = 100;
@:b = -100.4;

// will print a number approximately 0.4
print(message:a + b);

@:c = 0xf;
// will print 15
print(message:c);

// will likely print 6.66666666666667 or 
// higher accuracy value
print(message: a / c);


@:d = 3e10;

// Prints 30000000000
print(message:d);

