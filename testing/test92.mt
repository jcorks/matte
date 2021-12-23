//// Test92 
//
// Preserver
//
// This might behave differently across versions and 
// implementations since the language does not 
// currently specify when garbage collection 
// occurs
@resID = 0;
@reviveCount = 0;

@makePreservable = ::<={
    @reserved = [];
    @resCount = 0;
    
    
    return ::{
        @res = reserved;
        
        
        when(resCount != 0) ::<={
            @o = res[resCount-1];
            removeKey(from:res, key:resCount-1);
            resCount-=1;
            reviveCount += 1;

            @ops = getAttributes(of:o);
            ops.preserver = ::{
                reserved[resCount] = o;
                resCount+=1;        
            };
            setAttributes(of:o, attributes:ops);        
            return o;
        };
        
        
        
        
        
        @out = {
            resID : resID
        };
        
        resID = resID + 1;
        
        setAttributes(of:out, attributes:{
            preserver :: {
                reserved[resCount] = out;
                resCount+=1;
            }
        });
        return out;
    };
};



::<={
    @a = makePreservable();
};
::<={
    @a = makePreservable();
};

::<={
    @a = makePreservable();
};


return ''+reviveCount+resID;











