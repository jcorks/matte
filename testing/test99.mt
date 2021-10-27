//// Test 99 
//
// removeKey.

@keyobj = {};
@testObj = {
    'hello': 100,
    'hello1' : 101,
    '' : 10,
    (String) : 1000,
    (keyobj) : 4000,
    keyobj : 6000
};


@out = '';

listen(::{
    removeKey(100);
}, ::{
    out = out + 'noobj';
});


removeKey(testObj, 'hello');
out = out + String(testObj.hello);

removeKey(testObj, '');
removeKey(testObj, 'hello1');

out = out + testObj[String];
out = out + testObj[keyobj];

out = out + introspect.keycount(testObj);

removeKey(testObj, String);
removeKey(testObj, keyobj);
removeKey(testObj, 'keyobj');
removeKey(testObj, String); // ok

out = out + introspect.keycount(testObj);

return out;




