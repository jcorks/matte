////Test 84
//
// String: split


@a = 'This_is_a_string';

@output = '';
a->split(token:'_')->foreach(do:::(k, v) {
    output = output + v;
});

a->split(token:' ')->foreach(do:::(k, v) {
    output = output + v;
});

a->split(token:'is')->foreach(do:::(k, v) {
    output = output + v;
});


a->split(token:'This_is_a_string')->foreach(do:::(k, v) {
    output = output + v;
});

a->split(token:'This')->foreach(do:::(k, v) {
    output = output + v;
});


return output;
