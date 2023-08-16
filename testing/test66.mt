////Test 51
//
// Core: Class (5)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @str = 'isitworking';


        this.interface = {
            mutate :: {
                str = str + 'OHWOW2';
            },

            data : {
                get :: {
                    return str;
                }
            }
        }
    }
);

@instance  = TestClass.new();
instance.mutate();

return ''+instance.data;
