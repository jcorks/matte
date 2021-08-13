@Array <-(staticArray) {
    @arrBase = (<-{
        when(staticArray) : staticArray;
        return [];
    })();

    print(arrBase[2]);
    
    @object = {};
    @interface = {
        length : <-{
            return introspect(arrBase).keycount();
        },

        push : <-{
            return <-(obj) {
                arrBase[introspect(arrBase).size()-1] = obj;
            };
        },

        remove : <-{
            return <-(index) {
                arrBase[index] = empty;
            };
        },

        default: <-{return empty;}
    };
    object.toString =<- {
        @str = '[';
        <@>len = interface.length();
        for([0, len], <-(i){
            str = str + arrBase[i]; 
            when(i != len-1):<-{
                str = str + ',';
            }();
        });
        str = str + ']';
        return str;
    };
    print(object);

    object.accessor =<-(obj, key){
        return interface[key]();        
    };

    return object;
};


@arr = Array([1, 2, 3]);
print(arr);