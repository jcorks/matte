//// Test 34
//
// empty object
@r = {};
setOperator(r, {
    (String) ::{
        return 'Testing';
    }
});
return String(r);
