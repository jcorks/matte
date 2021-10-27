////Test 54
//
// Operator


@createNumber ::(val) {
    @valDecimal;
    @valInteger;
    // overly complex implementations are great for testing
    // ... or so i tell myself
    if (val < 0) ::<={
        valDecimal = introspect.abs(val - introspect.floor(val) - 1);
        valInteger = introspect.floor(val) + 1;
    } else ::<={
        valDecimal = val - introspect.floor(val);
        valInteger = introspect.floor(val);
    };

    @ref = {
        decimal  : valDecimal,
        integer  : valInteger
    };
    
    setOperator(ref, {
        '+' ::(a) {
            return createNumber(val + (a.decimal + a.integer));
        },

        '-' ::(a) {
            return createNumber(val - (a.decimal + a.integer));
        },

        '/' ::(a) {
            return createNumber(val / (a.decimal + a.integer));
        },

        '*' ::(a) {
            return createNumber(val * (a.decimal + a.integer));
        },

        (String) :: {
            return 'Integer:' + valInteger + ',Decimal:' + valDecimal; 
        }
    });
    
    return ref;
};


@a = createNumber(10.5);
@b = createNumber(2.5);
@c = createNumber(5.5);


@result = 'Unchanged';
listen(::{
    result = a % b;
    result = '' + (a - b * c);
});


return result;
