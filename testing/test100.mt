//// Test 100
//
// setAttributes

@out = '';

@test = {
    data : 0    
}

@op = {
    '+=' ::(value => Number) {
        test.data += value;
    }
}

test->setAttributes(attributes:op);

test +=  100;
test +=  10;
out = out + test.data;

test->setAttributes(attributes:op);
test +=  1;
out = out + test.data;

::?{
    10->setAttributes(:op);    
}=> {onError:::(message){
    out = out + 'noobj0';
}}


::?{
    test->setAttributes(:0);    
}=>{ onError:::(message){
    out = out + 'noobj1';
}}
test +=  1;
out = out + test.data;

op['+='] = ::(value => Number) {
    test.data -= value;
}

test +=  100;
out = out + test.data;

@op2 = {
    '+=' ::(value) {
        test.data = 777;    
    }
}

test->setAttributes(:op2);
test +=  33;
out = out + test.data;



out = out + Boolean(:test->attributes == op2);
::?{
    Object.getAttributes(:'d');
}=>{onError:::(message){
    out = out + 'noobjEX';
}}

out = out + Boolean(:test->attributes == op);




return out;





