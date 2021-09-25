////Test 84
//
// String: split


@Matte = import('Matte.Core');
@a = Matte.String.new('This_is_a_string');

@output = Matte.String.new();
foreach(a.split('_'), ::(k, v) {
    output += v;
});

foreach(a.split(' '), ::(k, v) {
    output += v;
});

foreach(a.split('is'), ::(k, v) {
    output += v;
});

foreach(a.split('This_is_a_string'), ::(k, v) {
    output += v;
});

return output;