
@enum = ::<={
    @enumID = 0;
    @auto = {};

    @out = ::(opts, name) {

        @enumtype = newtype({'name':if (name) name else ('MatteEnumerator'+enumID)});
        enumID = enumID+1;

        // creates an enum value.
        // uniquely identifiable and only comparable with other 
        // enums of the same type.
        @createEnumVal ::(numHint) {
            @val = instantiate(enumtype);
            val.numHint = numHint;
            

            
            setAttributes(val, {
                // throw an error if comparing against other non-enum types
                '==' :: (other => enumtype) {
                    return numHint == other.numHint;
                },
                
                
                (Number) :: {
                    return numHint;
                },
                
                
                '[]' :{
                    set ::(key){
                       error('Enumerator value has no properties.');
                    }
                },
                '.' :{
                    set ::(key){
                       error('Enumerator value has no properties.');
                    }
                }


            });

            return val;
        };




        @ptype = {};
        
        
        
        @autoNum = 0;
        @valToKey :: (val) {
            return match(introspect.type(val)) {
                (Number) : createEnumVal(val),
                (Object) : 
                    if(val == auto) ::{
                        @varOut = autoNum;
                        autoNum = autoNum + 1;
                        return createEnumVal(varOut);
                    }() else 
                        error('Cannot give an object as an enum value.'),
                default :
                    error('Cannot give a non-number type as an enum value')
            };
        };
        ptype['type'] = enumtype;
        
        foreach(opts, ::(key, val) {
            ptype[key] = valToKey(val);
            ptype[Number(ptype[key])] = ptype[key];
        });
        
        
        @output = {};
        @getter = ::(k) => enumtype {
            return ptype[k];
        };
        @setter = ::{
            error('Enum value is read-only');
        };

        setAttributes(
            output,
            {
                '[]': {
                    get : getter,
                    set : setter
                },
                '.': {
                    get : getter,
                    set : setter
                }
            }
        );

        return output;
    };
};

return enum;


