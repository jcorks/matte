return 0;
@a = 0;
@from = getExternalFunction(name:"system_getticks")();

@makeThing ::(index) {
    print(message:'' + index + '@ ' + (getExternalFunction(name:"system_getticks")() - from));
};

[::]{
    forever(do:::{
        @b = a/100;
        a+=1;
        if (a%10 == 0) ::<={
            when(a%100 == 0) ::<={
                makeThing(index:a);
                send();
            };
        };
        
        return true;
    });
};
