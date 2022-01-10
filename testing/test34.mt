//// Test 34
//
// empty object
@r = {};
Object.setAttributes(
    of:r, 
    attributes:{
        (String) ::{
            return 'Testing';
        }
    }
);
return String(from:r);
