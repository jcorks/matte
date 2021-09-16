////Test 50
//
// Core: Class (4)
@Class = import('Matte.Class');
@TestClass = Class({
    define ::(this, args, classinst) {
        @str = args;

        this.interface({
            mutate :: {
                str = str + 'OHWOW';
            },

            toString :: {
                return str;
            }
        });
    }
});

@instance  = TestClass.new('isitworking');
instance.mutate();

return ''+instance;