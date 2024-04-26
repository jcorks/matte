// Test 109
//
// key / value attributes 


@:test = {}


test->setAttributes(:{
        keys   :: <- [1, 2, 3, 4, 5],
        values :: <- [6, 7, 8, 9]
});



@out = '';

test->keys->foreach(::(index, value) {
    out = out + value;
});


test->values->foreach(::(index, value) {
    out = out + value;
});


return out;





