//// Test 100
//
// setAttributes

@out = '';

@test = {
    data : 0    
};

@op = {
    '+=' ::(i => Number) {
        test.data += i;
    }
};

setAttributes(test, op);

test +=  100;
test +=  10;
out = out + test.data;

setAttributes(test, op);
test +=  1;
out = out + test.data;

listen(::{
    setAttributes(10, op);    
}, ::{
    out = out + 'noobj0';
});


listen(::{
    setAttributes(test, 0);    
}, ::{
    out = out + 'noobj1';
});
test +=  1;
out = out + test.data;

op['+='] = ::(i => Number) {
    test.data -= i;
};

test +=  100;
out = out + test.data;

@op2 = {
    '+=' ::(i) {
        test.data = 777;    
    }
};

setAttributes(test, op2);
test +=  33;
out = out + test.data;



out = out + Boolean(getAttributes(test) == op2);
listen(::{
    getAttributes('d');
}, ::{
    out = out + 'noobjEX';
});

out = out + Boolean(getAttributes(test) == op);




return out;





