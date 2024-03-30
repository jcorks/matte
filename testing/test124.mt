//// Test 124
//
// Var args + dynamic binding 

@:f = ::($, a) {
    return $.x + a;
}


@:a = {
    x : 1000,
    fn : f
}

@:out = a.fn(*{$:{x:3000}, a:100});

return out;
