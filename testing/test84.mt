////Test 84
//
// String: split


@a = 'This_is_a_string';

@output = '';
a->split(:'_')->foreach(::(k, v) {
    output = output + v;
});

a->split(:' ')->foreach(::(k, v) {
    output = output + v;
});

a->split(:'is')->foreach(::(k, v) {
    output = output + v;
});


a->split(:'This_is_a_string')->foreach(::(k, v) {
    output = output + v;
});

a->split(:'This')->foreach(::(k, v) {
    output = output + v;
});


return output;
