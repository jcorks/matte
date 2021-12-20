<@>class = import(module:'Matte.Core.Class');


// Arrays contain a number-indexed collection of values.
@Array = class(info:{
    name : 'Matte.Core.Array',
    define::(this) {
        @data;
        @len;

        <@>initialize = ::(args){
            data = if(Boolean(from:args) && introspect.type(of:args) == Object) args else [];
            len = introspect.keycount(of:data);
        };

        this.constructor = ::(from) {
            initialize(args:from);
            return this;
        };

        this.recycle = true;

        this.interface = {
            onRevive : initialize,

            // Read/write variable.
            // Returns/Sets the length of the array.
            length : {
                get :: {
                    return len;
                },
                
                set ::(value) {
                    len = value;
                    @o = data[len-1];
                    data[len-1] = o;
                }
            },


            // Function, 1 argument. 
            // No return value.
            // Adds an additional element to the array.
            push :: (value) {
                data[len] = value;
                len = len + 1;
            },
            
            pop :: {
                when(len == 0) empty;
                @out = data[len-1];
                removeKey(from:data, key:len-1);
                len -= 1;
                return out;
            },
            
            insert ::(i, v) {
                when (i >= len) error(data:'No such index to insert at.');
                when (i == len-1) this.push(value:v);
                
                for(in:[i, len], do:::(i) {
                    data[i+1] = data[i];
                });
                data[i] = v;
                len += 1;
            },

            // Function, 1 argument.
            // Returns.
            // Removes the element at the specified index and returns it.
            // Like all array operations, the specified index is 
            // 0-indexed.
            remove ::(index => Number) {
                when(index < 0 || index >= len) empty;
                @out = data[index];
                removeKey(from:data, key:index);
                len -= 1;
                return out;
            },
            
            // Function, 1 argument 
            // No return value 
            // Removes the first element that equals the given value.
            removeValue ::(value) {
                listen(to:::{
                    for(in:[0, len], do:::(i) {
                        if (data[i] == value) ::<={
                            removeKey(from:data, key:i);
                            len -= 1;
                            send();
                        };
                    });
                });
            },


            // Function, no arguments.
            // Returns.
            // Creates a new array object that is a shallow copy of this one.
            clone :: {
                @out = this.class.new();
                for(in:[0, len], do:::(i){
                    out.push(value:data[i]);
                });
                return out;
            },

            // Function, one argument, optional.
            // No return value.
            // Re-arranges the contents of the array to be 
            // sorted according to the given comparator.
            // If no comparator is given, a default one using the 
            // "<" operator is used instead.
            sort ::(cmp) {
                // ensures a comparator exists if there isnt one.
                cmp = if(Boolean(from:cmp)) cmp else ::(a, b) {
                    when(a < b) -1;
                    when(a > b)  1;
                    return 0;
                };

                // casts to an integer
                @INT :: (n){
                    return introspect.floor(of:n);
                };

                // swaps 2 members of the data array
                @swap ::(ai, bi) {
                    @temp = data[ai];
                    data[ai] = data[bi];
                    data[bi] = temp;
                }; 

                // l/r pivots
                @left = 0;
                @right = len-1;


                @partition ::(left, right) {
                    @pivot = data[right];
                    @i = left-1;
                    
                    for(in:[left, right], do:::(j) {
                        when(cmp(a:data[j], b:pivot) < 0)::{
                            i = i + 1;
                            swap(ai:i, bi:j);
                        }();
                    });
                    swap(ai:i+1, bi:right);             
                    return i + 1;
                };

                @subsort :: (cmp, left, right) {
                    when(right <= left) empty;

                    @pi = partition(left:left, right:right);
                    
                    context(cmp:cmp, left:left, right:pi-1);
                    context(cmp:cmp, left:pi+1, right:right);
                };

                subsort(cmp:cmp, left:left, right:right);
            },

            // returns a new array thats a subset of
            subset ::(from, to) {
                @out = this.class.new();
                when(from >= to) out;
                
                from = if(from <  0)   0     else from;                
                to   = if(to   >= len) len-1 else from;                
                
                for(in:[from, to+1], do:::(i){
                    out.push(value:data[i]);
                });
                return out;
            },
            
            // Function, one argument.
            // Returns.
            // Returns a new array of all elements that meet the condition 
            // given. "cond" should be an array that accepts at least one argument.
            // The first argument is the value.
            filter ::(cond) {
                @out = this.class.new();
                foreach(in:data, do:::(v, k){
                    when(cond(item:v)) out.push(value:v);
                });            
                return out;
            },
            
            
            // linear search
            find ::(inval) {
                return listen(to:::{
                    for(in:[0, len], do:::(i){
                        if (data[i] == inval) ::<={
                            send(message:i);  
                        };
                    });                
                    return -1;
                });
            },
            
            binarySearch::(val, cmp) {
                error(data:"not done yet");
            },

            at::(i => Number) {
                return data[i];
            }


            
        };
        
        this.attributes = {
            '[]' : {
                get :: (key => Number){
                    return data[key];
                },
                
                set :: (key => Number, value) {
                    data[key] = value;
                }
            },

            'foreach' :: {
                return data;
            },

            (String) ::{
                @str = '[';
                for(in:[0, len], do:::(i){
                    <@> strRep = (String(from:data[i]));

                    str = str + if (strRep) strRep else ('<' + introspect.type(of:data[i]) + '>');

                    when(i != len-1)::{
                        str = str + ', ';
                    }();
                });
                return str + ']';
            }
        };
    }
});

// bootstrap reserves for speed 
::<={
    @a = [];
    @acount = 0;
    for(in:[0, 5], do:::(i) {
        a[acount] = Array.new();    
        acount += 1;
    });
};
return Array;
