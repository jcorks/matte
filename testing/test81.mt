//// Test81
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('abcdefgh');


@strout = '';
for([0, a.length], ::(i){
    strout = strout + a.charCodeAt(i);
});



return strout;
