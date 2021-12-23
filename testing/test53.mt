////Test 53
//
// Operator


@createNumber ::(from) {
    @val = from;
    @valDecimal;
    @valInteger;
    // overly complex implementations are great for testing
    // ... or so i tell myself
    if (val < 0) ::<={
        valDecimal = introspect.abs(of:val - introspect.floor(of:val) - 1);
        valInteger = introspect.floor(of:val) + 1;
    } else ::<={
        valDecimal = val - introspect.floor(of:val);
        valInteger = introspect.floor(of:val);
    };

    @ref = {
        decimal  : valDecimal,
        integer  : valInteger
    };
    
    setAttributes(of:ref, 
        attributes: {
            '+' ::(value) {
                return createNumber(from:val + (value.decimal + value.integer));
            },

            '-' ::(value) {
                return createNumber(from:val - (value.decimal + value.integer));
            },

            '/' ::(value) {
                return createNumber(from:val / (value.decimal + value.integer));
            },

            '*' ::(value) {
                return createNumber(from:val * (value.decimal + value.integer));
            },

            (String) :: {
                return 'Integer:' + valInteger + ',Decimal:' + valDecimal; 
            }
    });
    
    return ref;
};


@a = createNumber(from:10.5);
@b = createNumber(from:2.6);
@c = createNumber(from:5.1);

return '' + (a + b + c);




