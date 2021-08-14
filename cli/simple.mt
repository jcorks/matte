// A sample array class
@Array <-(staticArray) {
    @arrBase = gate(staticArray) staticArray : [];    
    @object = {};

    // For simplicity, the interface is a 
    // named property table.
    @interface = {
        // Returns the length of the array.
        length : <-{
            return introspect(arrBase).keycount();
        },

        // pushes a new value to the array.
        push : <-{
            return <-(obj) {
                arrBase[introspect(arrBase).keycount()] = obj;
            };
        },

        // removes the given index from the array.
        remove : <-{
            return <-(index) {
                removeKey(arrBase, index);
            };
        },

        default: <-{return empty;}
    };

    // Prints a pretty version of the array contents
    object.toString = <-{
        @str = '[';
        <@>len = interface.length();
        for([0, len], <-(i){
            @end = gate(i < len-1) ',' : '';
            str = str + arrBase[i] + end;
        });
        str = str + ']';
        return str;
    };

    // The accessor intercepts accesses to supplant the 
    // existing members of object with 
    // a custom interface.
    object.accessor = <-(obj, key) {
        return interface[key]();        
    };

    return object;
};


@arr = Array([1, 20, 4, 44]);
arr.push(66);
arr.remove(2);
return arr;