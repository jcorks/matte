////Test 84
//
// String: split


@Matte = import(module:'Matte.Core');
@a = Matte.String.new(from:'This_is_a_string');

@output = Matte.String.new();
foreach(in:a.split(token:'_'), do:::(k, v) {
    output += v;
});

foreach(in:a.split(token:' '), do:::(k, v) {
    output += v;
});

foreach(in:a.split(token:'is'), do:::(k, v) {
    output += v;
});


foreach(in:a.split(token:'This_is_a_string'), do:::(k, v) {
    output += v;
});

foreach(in:a.split(token:'This'), do:::(k, v) {
    output += v;
});


return output;