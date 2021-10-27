//// Test 101
//
// External function test.

@fn;
@out = '';
listen(::{
    fn = getExternalFunction();
    fn();
}, ::{
    out = out + 'nofn';
});

listen(::{
    fn = getExternalFunction({});
    fn();
}, ::{
    out = out + 'nostr';
});



fn = getExternalFunction('external_test!');
out = out + fn(4, 20);



return out;
