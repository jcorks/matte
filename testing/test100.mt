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
}, onMessage:::(message){
    out = out + 'noobj0';
});


listen(to:::{
    setAttributes(of:test, attributes:0);    
}, onMessage:::(message){
    out = out + 'noobj1';
});
test +=  1;
out = out + test.data;

op['+='] = ::(value => Number) {
    test.data -= value;
};

test +=  100;
out = out + test.data;

@op2 = {
    '+=' ::(value) {
        test.data = 777;    
    }
};

setAttributes(of:test, attributes:op2);
test +=  33;
out = out + test.data;



out = out + Boolean(from:getAttributes(of:test) == op2);
listen(to:::{
    getAttributes(of:'d');
}, onMessage:::(message){
    out = out + 'noobjEX';
});

out = out + Boolean(from:getAttributes(of:test) == op);




return out;





