//// Test 76
//
// String: length
@Matte = import('Matte.Core');

@s = Matte.String.new('I am going to the store');
s.replace('I', 'You');
s.replace('am', 'are');
s.replace('We', 'no one');
s.replace('store', 'market');
s.replace(' ', '-');
return (String(s));
