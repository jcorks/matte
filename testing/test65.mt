////Test 50
//
// Core: Class (4)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(info:{
    define::(this) {
        @str;
        this.constructor = ::(theString) {
            str = theString;  
            return this;          
        };

        this.attributes = {
            (String) :: {
                return str;
            }
        };

        this.interface = {
            mutate :: {
                str = str + 'OHWOW';
            }
        };
        
    }
});

@instance  = TestClass.new(theString:'isitworking');
instance.mutate();

return ''+instance;
