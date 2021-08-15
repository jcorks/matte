//// Test 28
//
// object test: literal
@g :: {
    @local = 'test';
    return {
        hb : ::{
            return local;
        }
    };
};
return g().hb();
