//// Test 33
//
// empty object
@r = {};
r.toString =<- {
    return 'Testing';
};
return String(r);