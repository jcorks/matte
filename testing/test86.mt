//// Test 86
// 
// Preserver!
//
@deleted = 0;

@createObject::{
    @out = {};
    
    setOperator(out, {
        preserver :: {
            deleted += 1;
        }           
    });
    
};

@test ::{
    @a = createObject();
    @b = createObject();
};


test();
test();

return deleted;
