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
    Object.removeKey(from:100);
}, onError:::(message){
    out = out + 'noobj';
});


Object.removeKey(from:testObj, key:'hello');
out = out + String(from:testObj.hello);

Object.removeKey(from:testObj, key:'');
Object.removeKey(from:testObj, key:'hello1');

out = out + testObj[String];
out = out + testObj[keyobj];

out = out + Object.keycount(of:testObj);

Object.removeKey(from:testObj, key:String);
Object.removeKey(from:testObj, key:keyobj);
Object.removeKey(from:testObj, key:'keyobj');
Object.removeKey(from:testObj, key:String); // ok

out = out + Object.keycount(of:testObj);

return out;




