////Test 51
//
// Core: Class (5)
@Class = import('Matte.Core.Class');
@TestClass = Class({
    define ::(this, args, classinst) {
        @str = args;

        this.interface({
            mutate :: {
                str = str + 'OHWOW2';
            },

            data : {
                get :: {
                    return str;
                }
            }
        });
    }
});

@instance  = TestClass.new('isitworking');
instance.mutate();

return ''+instance.data;
