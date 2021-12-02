////Test 52
//
// Core: Class (6)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class({
    define ::(this, thisType) {
        @data;

        this.constructor = ::(input) {
            data = input;
        };


        this.interface = {
            data : {
                get :: {
                    return data - 400;
                },

                set ::(v) {
                    data = 1000 + v;
                }
            }
        };
    }
});

@instance  = TestClass.new(input:1);
instance.data = 3;
return ''+instance.data;
