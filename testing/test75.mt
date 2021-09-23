//// Test 75
//
// String: length
@Matte = import('Matte.Core');

when(Matte.String.new().length != 0) 'failed0';
when(Matte.String.new('Hello').length != 5) 'failed1';
when(Matte.String.new('this is a string').length != 16) 'failed2';
when(Matte.String.new(Matte.String.new('ABC') + Matte.String.new('123')).length != 6) 'failed3';
    
    
@a = Matte.String.new('aaa');
a.length = 6;
return ''+a.length;
