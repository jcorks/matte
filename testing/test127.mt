//// Test 127 
//
// default parameters

@out = '';


@:fn::(a:4) {
    out = out + a;
}

fn();




@:testObject = ::<= {
    @v = 100;
    return {
        something :: {
            v += 10;
            return v;
        }
    }
};

@:function = ::(
    a => Number : 444, 
    b: testObject.something()
) {
    out = out + (a+b);
}

testObject.something();
function(a:1000);

print(:out);









