////Test 49
//
// Core: Class (3)
@Class = import(module:'Matte.Core.Class');
@TestClass = Class(
    define:::(this) {
        @args;

        this.constructor = ::(myArg) {
            args = myArg;
            this.instance->setAttributes(
                attributes : {
                    (String) :: {
                        return args;
                    }
                }
            );
            return this.instance;
        }

    }
);

@instance  = TestClass.new(myArg:'hello!'+2);
@instance2 = TestClass.new(myArg:'ddd!'+5);

return ''+instance+instance2;
