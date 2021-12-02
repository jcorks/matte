////Test 57
//
// decrement

@s = 0;
@c = 0;
for(in:[0, 10], do:::{
    s+=4;
    c-=1;
});
return ''+c+s;
