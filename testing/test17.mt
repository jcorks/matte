//// Test 17
//
// named function, medium test 
@f = ::{
    return 10 + 20;
}

// remember: equal sign is optional for function assignment
@g :: (a) {
    return a + 40;
}
// also remember: for single-argument calls, auto-binding can be used 
// which is the same as parameter binding, but you omit the name.
return g(:f()+10);
