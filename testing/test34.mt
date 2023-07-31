//// Test 34
//
// empty object
@r = {}
r->setAttributes( 
    attributes:{
        (String) ::{
            return 'Testing';
        }
    }
);
return String(from:r);
