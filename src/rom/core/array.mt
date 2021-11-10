<@>class = import('Matte.Class');



@Array = class({
    name : 'Matte.Array',
    define ::(this, args, classinst) {
        @data;
        @len;

        <@>initialize = ::(args){
            data = if(Boolean(args) && introspect.type(args) == Object) args else [];
            len = introspect.keycount(data);
        };
        initialize(args);
        this.interface({
            onRevive : initialize,

            // Read/write variable.
            // Returns/Sets the length of the array.
            length : {
                get :: {
                    return len;
                },
                
                set ::(newV) {
                    len = newV;
                    @o = data[len-1];
                    data[len-1] = o;
                }
            },


            // Function, 1 argument. 
            // No return value.
            // Adds an additional element to the array.
            push :: (nm) {
                data[len] = nm;
                len = len + 1;
            },
            
            pop :: {
                when(len == 0) empty;
                @out = data[len-1];
                removeKey(data, len-1);
                len -= 1;
                return out;
            },

            // Function, 1 argument.
            // Returns.
            // Removes the element at the specified index and returns it.
            // Like all array operations, the specified index is 
            // 0-indexed.
            remove ::(nm) {
                when(nm < 0 || nm >= len) empty;
                @out = data[nm];
                removeKey(data, nm);
                return out;
            },


            // Function, no arguments.
            // Returns.
            // Creates a new array object that is a shallow copy of this one.
            clone :: {
                @out = classinst.new();
                for([0, len], ::(i){
                    out.push(data[i]);
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
                cmp = if(Boolean(cmp)) cmp else ::(a, b) {
                    when(a < b) -1;
                    when(a > b)  1;
                    return 0;
                };

                // casts to an integer
                @INT :: (n){
                    return introspect.floor(n);
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
                    
                    for([left, right], ::(j) {
                        when(cmp(data[j], pivot) < 0)::{
                            i = i + 1;
                            swap(i, j);
                        }();
                    });
                    swap(i+1, right);             
                    return i + 1;
                };

                @subsort :: (cmp, left, right) {
                    when(right <= left) empty;

                    @pi = partition(left, right);
                    
                    context(cmp, left, pi-1);
                    context(cmp, pi+1, right);
                };

                subsort(cmp, left, right);
            },

            // returns a new array thats a subset of
            subset ::(from, to) {
                @out = classinst.new();
                when(from >= to) out;
                
                from = if(from <  0)   0     else from;                
                to   = if(to   >= len) len-1 else from;                
                
                for([from, to+1], ::(i){
                    out.push(data[i]);
                });
                return out;
            },
            
            // Function, one argument.
            // Returns.
            // Returns a new array of all elements that meet the condition 
            // given. "cond" should be an array that accepts at least one argument.
            // The first argument is the value.
            filter ::(cond) {
                @out = classinst.new();
                foreach(data, ::(v){
                    when(cond(v)) out.push(v);
                });            
                return out;
            },
            
            
            // linear search
            find ::(inval) {
                return listen(::{
                    for([0, len], ::(i){
                        if (data[i] == inval) ::<={
                            send(i);  
                        };
                    });                
                    return -1;
                });
            },
            
            binarySearch::(val, cmp) {
                error("not done yet");
            },

            at::(i => Number) {
                return data[i];
            }


            
        });
        
        this.attributes({
            '[]' : {
                get :: (key => Number){
                    return data[key];
                },
                
                set :: (key => Number, val) {
                    data[key] = val;
                }
            },

            'foreach' :: {
                return data;
            },

            (String) ::{
                @str = '[';
                for([0, len], ::(i){
                    <@> strRep = ::{
                        context.catch = ::{};
                        return (String(data[i]));
                    }();

                    str = str + if (strRep) strRep else ('<' + introspect.type(data[i]) + '>');

                    when(i != len-1)::{
                        str = str + ', ';
                    }();
                });
                return str + ']';
            }
        });
    }
});
return Array;
