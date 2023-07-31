//// Test 20
//
// value capture test 
@y = 21;
@f = ::{
    @g ::{
        return y;
    }
    return g;
}
return f()()+10;
