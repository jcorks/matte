//// Test 100
//
// setAttributes

@out = '';

@test = {
    data : 0    
};

@op = {
    '+=' ::(value => Number) {
        test.data += value;
    }
};

setAttributes(of:test, attributes:op);

test +=  100;
test +=  10;
out = out + test.data;

setAttributes(of:test, attributes:op);
test +=  1;
out = out + test.data;

listen(to:::{
    setAttributes(of:10, attributes:op);    
}, ::{
    out = out + 'noobj0';
});


listen(to:::{
    setAttributes(of:test, attributes:0);    
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

setAttributes(of:test, attributes:op2);
test +=  33;
out = out + test.data;



out = out + Boolean(from:getAttributes(of:test) == op2);
listen(to:::{
    getAttributes(of:'d');
}, onMessage:::{
    out = out + 'noobjEX';
});

out = out + Boolean(from:getAttributes(of:test) == op);




return out;





