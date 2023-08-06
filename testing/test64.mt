////Test 49
//
// Core: Class (3)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @args;

        this->setAttributes(
            attributes : {
                (String) :: {
                    return args;
                }
            }
        );
        
        
        this.interface = {
            init::(myArg) {
                args = myArg
                return this;
            }
        }

    }
);

@instance  = TestClass.new().init(myArg:'hello!'+2);
@instance2 = TestClass.new().init(myArg:'ddd!'+5);

return ''+instance+instance2;
