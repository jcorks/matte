//// Test 123 
//
// Dynamic binding

@:fn = ::($, a, b) {
    return $.x + a + b;
}


return ::<= {
    @a = {x : 1000};
    @b = {
        x: 10,
        fn : fn
    }
    
    a.fn = b.fn;
    @:out = a.fn(a:2, b:1);
    return out;
}
