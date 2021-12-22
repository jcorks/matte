//// Test 76
//
// String: length
@Matte = import(module:'Matte.Core');

@s = Matte.String.new(from:'I am going to the store');
s.replace(key:'I', with:'You');
s.replace(key:'am', with:'are');
s.replace(key:'We', with:'no one');
s.replace(key:'store', with:'market');
s.replace(key:' ', with:'-');
return (String(from:s));
