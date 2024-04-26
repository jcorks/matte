// test 108
//
// inline functions 


@:incrementer ::(value) <- value+1;


//3
@out = '' + incrementer(:2);




@:setIsBelow10 ::(set) <- (set->all(::(value) <- value < 10));

// false, since 11 
out = out + setIsBelow10(:[2, 11, 4, 5]);


// true
out = out + setIsBelow10(:[2, 1, 4]);

// true
out = out + setIsBelow10(:[2]);


return out;
