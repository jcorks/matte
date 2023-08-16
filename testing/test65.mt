////Test 50
//
// Core: Class (4)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @str;

        this->setAttributes(
            attributes : {
                (String) :: {
                    return str;
                }
            }
        );


        this.interface = {
            val : {
                set::(value) <- str = value
            },
            mutate :: {
                str = str + 'OHWOW';
            }
        }
        
    }
);

@instance  = TestClass.new();
instance.val = 'isitworking';
instance.mutate();

return ''+instance;
