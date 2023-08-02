////Test 50
//
// Core: Class (4)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @str;
        this.constructor = ::(theString) {
            str = theString;  
            this.instance->setAttributes(
                attributes : {
                    (String) :: {
                        return str;
                    }
                }
            );
            return this.instance;          
        }


        this.interface = {
            mutate :: {
                str = str + 'OHWOW';
            }
        }
        
    }
);

@instance  = TestClass.new(theString:'isitworking');
instance.mutate();

return ''+instance;
