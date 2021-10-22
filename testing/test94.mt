@a = 0;
@from = getExternalFunction("system_getticks")();

@makeThing ::(i) {
    print('' + i + '@ ' + (getExternalFunction("system_getticks")() - from));
};

loop(::{
    @b = a/100;
    a+=1;
    if (a%10 == 0) ::<={
        when(a%100 == 0) ::<={
           return makeThing(a);
        };
    };
    
    return true;
});
