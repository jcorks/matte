////Test 52
//
// Core: Class (6)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define: ::(this) {
        @data = '200';


        this.interface = {
            init ::(input) {
                data = input;
                return this;
            },
        
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

@instance  = TestClass.new().init(input:1);
instance.data = 3;
return ''+instance.data;
