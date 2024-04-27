//// Test 34
//
// empty object
@r = {}
r->setAttributes( 
    :{
        (String) ::{
            return 'Testing';
        }
    }
);
return String(:r);
