//// Test92 
//
// Preserver
//
// This might behave differently across versions and 
// implementations since the language does not 
// currently specify when garbage collection 
// occurs
@makePreservable = ::{
    @mp = context;
    @res = mp.reserved;
    
    if (!Boolean(res)) ::<={
        mp.reserved = [];
        mp.resCount = 0;
        mp.resID = 0;
        mp.reviveCount = 0;
        res = mp.reserved;
    };
    
    
    when(mp.resCount != 0) ::<={
        @o = res[mp.resCount-1];
        removeKey(res, mp.resCount-1);
        mp.resCount-=1;
        mp.reviveCount += 1;

        @ops = getOperator(o);
        ops.preserver = ::{
            mp.reserved[mp.resCount] = o;
            mp.resCount+=1;        
        };
        setOperator(o, ops);        
        return o;
    };
    
    
    
    
    
    @out = {
        resID : mp.resID
    };
    
    mp.resID = mp.resID + 1;
    
    setOperator(out, {
        preserver :: {
            mp.reserved[mp.resCount] = out;
            mp.resCount+=1;
        }
    });
    return out;
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


return ''+makePreservable.reviveCount+makePreservable.resID;











