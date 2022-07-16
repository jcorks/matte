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

[::]{
    100->remove();
}: {onError:::(message){
    out = out + 'noobj';
}};


testObj->remove(key:'hello');
out = out + String(from:testObj.hello);

testObj->remove(key:'');
testObj->remove(key:'hello1');

out = out + testObj[String];
out = out + testObj[keyobj];

out = out + testObj->keycount;;

testObj->remove(key:String);
testObj->remove(key:keyobj);
testObj->remove(key:'keyobj');
testObj->remove(key:String); // ok

out = out + testObj->keycount;

return out;




