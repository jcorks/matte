//// Test 76
//
// String: length

@s = 'I am going to the store';
s = s->replace(key:'I', with:'You');
s = s->replace(key:'am', with:'are');
s = s->replace(key:'We', with:'no one');
s = s->replace(key:'store', with:'market');
s = s->replace(key:' ', with:'-');
return s;
