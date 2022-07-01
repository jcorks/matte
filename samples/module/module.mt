@:class = import(module:'Matte.Core.Class');
@:offset = Number(from:parameters.offset);

return class(
    define :: (this) {
        // declare local variables
        @value0 = 0;
        @value1 = 0;
        @value2 = 0;
    
        // constants
        @:constant = 1;
	breakpoint();


        this.constructor = ::(initial) {
            value2 = if (initial != empty) initial else empty; 
            return this;
        };

        this.interface = {
            // public variable, write-only
            value0 : {
                set :: (value) {
                    value0 = value;
                }
            },

            // public variable, write-only            
            value1 : {
                set :: (value) {
                    value1 = value;
                }
            },
            
            // public function, adds both variables
            add :: {
                return value0 + value1 + constant + offset;
            }
        };
    }
);
