//// Test 17
//
// named function, medium test 
@f = <-{
    return 10 + 20;
};

// remember: equal sign is optional for function assignment
@g <- (a) {
    return a + 40;
};
return g(f()+10);