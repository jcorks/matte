//// Test 101
//
// External function test.

@fn;
@out = '';
listen(to:::{
    fn = getExternalFunction();
    fn();
}, onError:::(message){
    out = out + 'nofn';
});

listen(to:::{
    fn = getExternalFunction(name:{});
    fn();
}, onError:::(message){
    out = out + 'nostr';
});



fn = getExternalFunction(name:'external_test!');
out = out + fn(a:4, b:20);



return out;
