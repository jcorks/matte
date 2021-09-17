//// Test 34
//
// empty object
@r = {};
r.operator = {
    (String) ::{
        return 'Testing';
    }
};
return String(r);
