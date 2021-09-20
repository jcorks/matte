////Test 57
//
// decrement

@s = 0;
@c = 0;
for([0, 10], ::{
    s-=1;
    s+=1;
});
return ''+s+c;