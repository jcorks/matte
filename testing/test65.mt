////Test 50
//
// Core: Class (4)
@Class = import('Matte.Class');
@TestClass = Class({
    define ::(this, args, classinst) {
        @str = args;

        this.operator({
            (String) :: {
                return str;
            }
        });

        this.interface({
            mutate :: {
                str = str + 'OHWOW';
            }
        });
        
    }
});

@instance  = TestClass.new('isitworking');
instance.mutate();

return ''+instance;
