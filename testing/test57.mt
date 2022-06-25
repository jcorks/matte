////Test 57
//
// decrement

@s = 0;
@c = 0;
[0, 10]->for(do:::{
    s+=4;
    c-=1;
});
return ''+c+s;
