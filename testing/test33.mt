//// Test 33
//
// for loop
@iter = 0;
@iter2 = 0;
for(in:[0, 10010], do:::(i){
    iter = i;
    iter2 = iter2+1;
});
return iter2+iter;