//// Test 34
//
// empty object
@r = {};
r.toString = ::{
    return 'Testing';
};
return AsString(r);