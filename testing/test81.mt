//// Test81
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdefgh');
@arr = a.valueize();


@strout = '';
for([0, arr.length], ::(i) {
    strout = strout + arr[i];
});

strout = strout + Matte.String.new().valueize().length;


return strout;
