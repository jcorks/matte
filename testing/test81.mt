//// Test81
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdefgh');
@arr = a.valueize();


@strout = '';
for([0, arr.length], ::(i) {
    strout = strout + arr.at(i);
});


return strout;
