////Test 84
//
// String: split


@a = 'This_is_a_string';

@output = '';
foreach(in:String.split(string:a, token:'_'), do:::(k, v) {
    output = output + v;
});

foreach(in:String.split(string:a, token:' '), do:::(k, v) {
    output = output + v;
});

foreach(in:String.split(string:a, token:'is'), do:::(k, v) {
    output = output + v;
});


foreach(in:String.split(string:a, token:'This_is_a_string'), do:::(k, v) {
    output = output + v;
});

foreach(in:String.split(string:a, token:'This'), do:::(k, v) {
    output = output + v;
});


return output;
