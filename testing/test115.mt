//// Test 115
// 
// subset

@out = '';
out = String.combine(strings:(['H','e','l','l','o',',']->subset(from:2, to:5)));

out = out + (["1"]->subset(from:0, to:0))[0];



out = out + (['11', '22', '33', '44']->subset(from:1, to:3))[0];

out = out + [0]->subset(from:0, to:2);


return out;
