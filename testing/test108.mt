// test 108
//
// inline functions 


@:incrementer ::(value) <- value+1;


//3
@out = '' + incrementer(value:2);




@:setIsBelow10 ::(set) <- (set->all(condition:::(value) <- value < 10));

// false, since 11 
out = out + setIsBelow10(set:[2, 11, 4, 5]);


// true
out = out + setIsBelow10(set:[2, 1, 4]);

// true
out = out + setIsBelow10(set:[2]);


return out;
