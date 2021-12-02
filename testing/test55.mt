////Test 54
//
// Operator


@createNumber ::(value) {
    @valDecimal;
    @valInteger;
    // overly complex implementations are great for testing
    // ... or so i tell myself
    if (value < 0) ::<={
        valDecimal = introspect.abs(of:value - introspect.floor(of:value) - 1);
        valInteger = introspect.floor(of:value) + 1;
    } else ::<={
        valDecimal = value - introspect.floor(value);
        valInteger = introspect.floor(value);
    };

    @ref = {
        decimal  : valDecimal,
        integer  : valInteger
    };
    
    setAttributes(ref, {
        '+' ::(a) {
            return createNumber(value:value + (a.decimal + a.integer));
        },

        '-' ::(a) {
            return createNumber(value:value - (a.decimal + a.integer));
        },

        '/' ::(a) {
            return createNumber(value:value / (a.decimal + a.integer));
        },

        '*' ::(a) {
            return createNumber(value:value * (a.decimal + a.integer));
        },

        (String) :: {
            return 'Integer:' + valInteger + ',Decimal:' + valDecimal; 
        }
    });
    
    return ref;
};


@a = createNumber(value:10.5);
@b = createNumber(value:2.5);
@c = createNumber(value:5.5);


@result = 'Unchanged';
listen(to:::{
    result = a % b;
    result = '' + (a - b * c);
});


return result;
