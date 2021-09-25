//// Test82
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdefgh');
@arr = a.characterize();


@strout = '';
for([0, arr.length], ::(i) {
    strout = strout + arr[i];
});

strout = strout + Matte.String.new().valueize().length;


return strout;
