//// Test82
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdefgh');


@strout = '';
foreach(a, ::(k, v) {
    strout = strout + v;
});


return strout;
