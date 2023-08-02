////Test 52
//
// Core: Class (6)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define: ::(this) {
        @data;

        this.constructor = ::(input) {
            data = input;
            return this.instance;
        }


        this.interface = {
            data : {
                get :: {
                    return data - 400;
                },

                set ::(value) {
                    data = 1000 + value;
                }
            }
        }
    }
);

@instance  = TestClass.new(input:1);
instance.data = 3;
return ''+instance.data;
