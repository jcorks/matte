//// Test 76
//
// String: length

@s = 'I am going to the store';
s = String.replace(string:s, key:'I', with:'You');
s = String.replace(string:s, key:'am', with:'are');
s = String.replace(string:s, key:'We', with:'no one');
s = String.replace(string:s, key:'store', with:'market');
s = String.replace(string:s, key:' ', with:'-');
return s;
