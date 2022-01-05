//// Test 75
//
// String: length
@Matte = import(module:'Matte.Core');

when(Matte.String.new().length != 0) 'failed0';
when(Matte.String.new(from:'Hello').length != 5) 'failed1';
when(Matte.String.new(from:'this is a string').length != 16) 'failed2';
    
    
@a = Matte.String.new(from:'aaa');
a.length = 6;
return ''+a.length;
