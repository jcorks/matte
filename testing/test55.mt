////Test 54
//
// Operator


@createUSD ::(from) {
    // overly complex implementations are great for testing
    // ... or so i tell myself
    @isNeg = from < 0;
    from = Number.abs(of:from);
    
    @ref = {
        cents    : Number.round(value:100*(from - Number.floor(of:from))),
        dollars  : Number.floor(of:from),
        isDebt   : isNeg
    };
    
    @:toValue::(USD) {
        return (USD.dollars + USD.cents/100) * (if (USD.isDebt) -1 else 1);
    };
    
    
    Object.setAttributes(of:ref, 
        attributes: {
            '+' ::(value) {
                return createUSD(from:toValue(USD:ref) + toValue(USD:value));
            },

            '-' ::(value) {
                return createUSD(from:toValue(USD:ref) - toValue(USD:value));
            },

            '*' ::(value => Number) {
                return createUSD(from:toValue(USD:ref) * value);
            },
            
            (String) :: {
                return 'Dollars:' + ref.dollars + ',Cents:' + ref.cents + ',IsDebt:' + ref.isDebt; 
            }
    });
    
    return ref;
};



@a = createUSD(from:10.5);
@b = createUSD(from:2.5);
@c = createUSD(from:5.5);


@result = 'Unchanged';
listen(to:::{
    result = a % b;
    result = '' + (a - b * 2);
}, onError:::(message) {

});


return result;
