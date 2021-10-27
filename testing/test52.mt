////Test 52
//
// Typestrict


@fn ::(a) => Boolean {
    when(a > 10) false;
    return 100;
};


@result = 'ee' + fn(11); // ok 

listen(::{


    result = result + fn(100); //
    result = result + fn(0); // NOT OK
    result = result + fn(1000); // ok
    
});


return result;
