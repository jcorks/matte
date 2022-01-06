////Test 49
//
// Core: Class (3)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @args;

        this.constructor = ::(myArg) {
            args = myArg;
            return this;
        };

        this.attributes = {
            (String) :: {
                return args;
            }
        };
    }
);

@instance  = TestClass.new(myArg:'hello!'+2);
@instance2 = TestClass.new(myArg:'ddd!'+5);

return ''+instance+instance2;
