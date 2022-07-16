////Test 52
//
// Typestrict


@fn ::(a) => Boolean {
    when(a > 10) false;
    return 100;
};


@result = 'ee' + fn(a:11); // ok 


[::]{
    result = result + fn(a:100); //
    result = result + fn(a:0); // NOT OK
    result = result + fn(a:1000); // ok
    
} : {
    onError:::(message) {
        // caught, but doesnt do anything
    }
};


return result;
