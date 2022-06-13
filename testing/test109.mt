// Test 109
//
// key / value attributes 


@:test = {};


test->setAttributes(
    attributes: {
        keys   :: <- [1, 2, 3, 4, 5],
        values :: <- [6, 7, 8, 9]
    }
);



@out = '';

foreach(in:test->keys, do:::(index, value) {
    out = out + value;
});


foreach(in:test->values, do:::(index, value) {
    out = out + value;
});


return out;





