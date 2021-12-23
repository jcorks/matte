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

listen(to:::{
    removeKey(from:100);
}, onMessage:::(message){
    out = out + 'noobj';
});


removeKey(from:testObj, key:'hello');
out = out + String(from:testObj.hello);

removeKey(from:testObj, key:'');
removeKey(from:testObj, key:'hello1');

out = out + testObj[String];
out = out + testObj[keyobj];

out = out + introspect.keycount(of:testObj);

removeKey(from:testObj, key:String);
removeKey(from:testObj, key:keyobj);
removeKey(from:testObj, key:'keyobj');
removeKey(from:testObj, key:String); // ok

out = out + introspect.keycount(of:testObj);

return out;




