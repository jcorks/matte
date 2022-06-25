// Test 107
//
// spread operator (destructuring)

@:A = ['d', 'e', 'f'];

@:B = ['a', 'b', 'c', ...A, 'g'];

@:C = {
	...B, 
	'test' : '|', 
	...::<={
	    @:test = {};
	    test['AA'] = 'A';
	    test['BB'] = 'B';
	    test['CC'] = 'C';
        return test;
	}
};
@:D = {
    ...C,
    ...A
};

@out = '';
B->foreach(do:::(key, v) {
    out = out + v;
});

out = out + C['AA']; // A
out = out + C[5]; // f
out = out + D[1]; // e (since A overwrites old entry)

return out;
