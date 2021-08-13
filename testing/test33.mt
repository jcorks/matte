//// Test 33
//
// for loop
@iter = 0;
for([0, 10010], <-{
    iter = iter+1;
});
return iter;