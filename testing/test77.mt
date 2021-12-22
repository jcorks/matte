//// Test77 
//
// 
@Matte = import(module:'Matte.Core');

@a = Matte.String.new(from:'987437225794302230422');
return ''+
        a.count(key:'4') + 
        a.count(key:'7') + 
        a.count(key:'22') + 
        a.count(key:'a') + 
        a.count(key:'987437225794302230422');
