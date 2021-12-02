////Test 52
//
// Typestrict


@fn ::(a) => Boolean {
    when(a > 10) false;
    return 100;
};


@result = 'ee' + fn(a:11); // ok 

listen(to: ::{
    result = result + fn(a:100); //
    result = result + fn(a:0); // NOT OK
    result = result + fn(a:1000); // ok
    
});


return result;
