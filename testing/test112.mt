//// Test 112
//
// Test of expression with assignment

@out = '';

@a ::(b) <- b = 12;
out = out + a(b:'hello!');

out = out + (a = 13);

@b = 0;
out = out + ((a += 2) - (b = 5));

out = out + a;
out = out + b;
return out;

