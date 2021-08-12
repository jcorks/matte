//// Test 21
//
// value capture test 
@f = <-{
    @y = 22;
    @g <- {
        return y;
    };
    return g;
};
return f()()+10;