//// Test 116 
//
//  map, reduce, filter


@a = [1, 2, 3, 4, 5, 6]->filter(::(value) <- value > 3);
@out = '' + a[0] + a[1] + a[2];
a = a->map(::(value) <- '-'+value);
out = out + a[0] + a[1] + a[2];
out = out + a->reduce(::(previous, value) <- value + previous);  
return out;
