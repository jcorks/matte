
@enum ::(opts, name) {

    @enumtype = newtype({'name':if (name) name else ('MatteEnumerator'+context.enumID)});
    context.enumID = context.enumID+1;

    // creates an enum value.
    // uniquely identifiable and only comparable with other 
    // enums of the same type.
    @createEnumVal ::(numHint) {
        @val = instantiate(enumtype);
        val.numHint = numHint;
        

        
        val.operator = {
            // throw an error if comparing against other non-enum types
            '==' :: (other => enumtype) {
                return numHint == other.numHint;
            },
            
            
            (Number) :: {
                return numHint;
            }
        };



        val.setter = ::(key){
            error('Enumerator value has no properties.');
        };    
        return val;
    };




    @ptype = {};
    
    
    
    @autoNum = 0;
    @valToKey :: (val) {
        return match(introspect.type(val)) {
            (Number) : createEnumVal(val),
            (Object) : 
                if(val == enum.auto) ::{
                    @varOut = autoNum;
                    autoNum = autoNum + 1;
                    return createEnumVal(varOut);
                }() else 
                    error('Cannot give an object as an enum value.'),
            default :
                error('Cannot give a non-number type as an enum value')
        };
    };
    
    foreach(opts, ::(key, val) {
        ptype[key] = valToKey(val);
    });
    
    
    @output = {};
    output.getter = ::(k) => enumtype {
        return ptype[k];
    };
    output.setter = ::{
        error('Enum value is read-only');
    };

    return output;
};
enum.enumID = 0;
enum.auto = {};

return enum;


