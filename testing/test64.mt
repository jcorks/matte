////Test 49
//
// Core: Class (3)
@Class = import('Matte.Core.Class');
@TestClass = Class({
    define ::(this, args, classinst) {
        this.attributes({
            (String) :: {
                return args;
            }
        });
    }
});

@instance  = TestClass.new('hello!'+2);
@instance2 = TestClass.new('ddd!'+5);

return ''+instance+instance2;
