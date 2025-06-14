//// Test 99 
//
// removeKey.

@keyobj = {}
@testObj = {
    'hello': 100,
    'hello1' : 101,
    '' : 10,
    (String) : 1000,
    (keyobj) : 4000,
    keyobj : 6000
}


@out = '';

::?{
    100->remove();
}: {onError:::(message){
    out = out + 'noobj';
}}


testObj->remove(:'hello');
out = out + String(:testObj.hello);

testObj->remove(:'');
testObj->remove(:'hello1');

out = out + testObj[String];
out = out + testObj[keyobj];

out = out + testObj->keycount;;

testObj->remove(:String);
testObj->remove(:keyobj);
testObj->remove(:'keyobj');
testObj->remove(:String); // ok

out = out + testObj->size;

return out;




