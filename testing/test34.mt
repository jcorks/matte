//// Test 34
//
// empty object
@r = {};
setAttributes(r, {
    (String) ::{
        return 'Testing';
    }
});
return String(r);
