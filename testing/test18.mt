//// Test 18
//
// value capture test 
@y = 100;
@f = <-{
    return y + 20;
};
return f();