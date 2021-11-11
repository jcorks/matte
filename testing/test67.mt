////Test 52
//
// Core: Class (6)
@Class = import('Matte.Core.Class');
@TestClass = Class({
    define ::(this, args, classinst) {
        @data = args;

        this.interface({
            data : {
                get :: {
                    return data - 400;
                },

                set ::(v) {
                    data = 1000 + v;
                }
            }
        });
    }
});

@instance  = TestClass.new(1);
instance.data = 3;
return ''+instance.data;
