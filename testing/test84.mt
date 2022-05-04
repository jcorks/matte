////Test 84
//
// String: split


@a = 'This_is_a_string';

@output = '';
foreach(in:a->split(token:'_'), do:::(k, v) {
    output = output + v;
});

foreach(in:a->split(token:' '), do:::(k, v) {
    output = output + v;
});

foreach(in:a->split(token:'is'), do:::(k, v) {
    output = output + v;
});


foreach(in:a->split(token:'This_is_a_string'), do:::(k, v) {
    output = output + v;
});

foreach(in:a->split(token:'This'), do:::(k, v) {
    output = output + v;
});


return output;
