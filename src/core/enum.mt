@class = import('Matte.Class');


@enum ::(name, opts) {
    // use this object as a key
    context.auto = {};

    @enumtype = type({'name':name});


    // creates an enum value.
    // uniquely identifiable and only comparable with other 
    // enums of the same type.
    @createEnumVal ::(numHint) {
        @val = instantiate(enumtype);
        val.numHint = numHint;
        
        val.getter = ::() {
            error('Enumerator value has no properties.');
        };    

        val.setter = ::(key){
            error('Enumerator value has no properties.');
        };    
        
        
        val.operator = {
            // throw an error if comparing against other non-enum types
            '==' :: (other => enumtype) {
                return numHint == enumtype.numHint;
            },
            
            
            Number :: {
                return numHint;
            }
        };
    }



    @ptype = {};
    
    
    
    @autoNum = 0;
    @valToKey :: (val) {
        return match(introspect(val).type()) {
            (Number) : createEnumVal(val),
            (Object) : 
                if(val == context.auto) ::{
                    @varOut = autoNum;
                    autoNum = autoNum + 1;
                    return createEnumVal(varOut);
                }() else 
                    error('Cannot give an object as an enum value.'),
            default :
                error('Cannot give a non-number type as an enum value')
        };
    }
    
    foreach(opts, ::(key, val) {
        ptype[key] = valToKey(val);
    });
    
    
    @output = {};
    output.assigner = ::{
        error('Enum value is read-only');
    };
    output.getter = ::(k) => enumtype {
        return ptype[k];
    };
}





/*
@myEnum = enum('myEnum', {
    SomeValue : enum.auto,
    SomeValue1 : enum.auto,
    SomeValue2 : enum.auto,
    SomeValue3 : enum.auto,
    SomeValue4 : enum.auto
});*/
