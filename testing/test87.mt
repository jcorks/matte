//// Test 87
// 
// Preserver!
//
@deletedOnce = false;
@saveMe;

@createObject::{
    @out = {};
    
    setOperator(out, {
        preserver :: {
            if (!deletedOnce) ::{
                deletedOnce = true;
            }() else ::{
                saveMe = out;
            }();
        },
        
        (String) :: {
            return 'TestSuccess';
        }
    });
    
};

@test ::{
    @a = createObject();
    @b = createObject();
};


test();

return ''+saveMe+deletedOnce;
