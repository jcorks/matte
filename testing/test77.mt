//// Test77 
//
// 
@Matte = import('Matte.Core');

@a = Matte.String.new('987437225794302230422');
return ''+
        a.count('4') + 
        a.count('7') + 
        a.count('22') + 
        a.count('a') + 
        a.count('987437225794302230422');
