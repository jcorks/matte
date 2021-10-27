//// Test 100
//
// setOperator

@out = '';

@test = {
    data : 0    
};

@op = {
    '+=' ::(i => Number) {
        test.data += i;
    }
};

setOperator(test, op);

test +=  100;
test +=  10;
out = out + test.data;

setOperator(test, op);
test +=  1;
out = out + test.data;

listen(::{
    setOperator(10, op);    
}, ::{
    out = out + 'noobj0';
});


listen(::{
    setOperator(test, 0);    
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

setOperator(test, op2);
test +=  33;
out = out + test.data;



out = out + Boolean(getOperator(test) == op2);
listen(::{
    getOperator('d');
}, ::{
    out = out + 'noobjEX';
});

out = out + Boolean(getOperator(test) == op);




return out;





