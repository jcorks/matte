//// Test81
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'abcdefgh');


@strout = '';
for(in:[0, a.length], do:::(i){
    strout = strout + a.charCodeAt(index:i);
});



return strout;
