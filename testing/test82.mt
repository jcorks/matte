//// Test82
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'abcdefgh');


@strout = '';
foreach(in:a, do:::(k, v) {
    strout = strout + v;
});


return strout;
