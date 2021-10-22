<@>class = import('Matte.Class');

return class({
    constructor :: (obj, args) {
        // declare local variables
        @value0 = 0;
        @value1 = 0;
        @value2 = if (args.value2 != empty) args.value2 else empty; 
    
        // constants
        <@>constant = 1;

        obj.interface({
            // public variable, write-only
            value0 : {
                set :: (v) {
                    value0 = v;
                }
            },

            // public variable, write-only            
            value1 : {
                set :: (v) {
                    value1 = v;
                }
            },
            
            // public function, adds both variables
            add :: {
                return value0 + value1 + constant;
            }
        });
    }
});
