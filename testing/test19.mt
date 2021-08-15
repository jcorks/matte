//// Test 19
//
// function-object test
@y = 21;
@f = ::{
    @g ::{
        return 10+70;
    };
    return g;
};
return f()()+10;