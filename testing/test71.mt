////Test 56
//
// Core: String (4)
<@>MatteString = import('Matte.String');

@str = MatteString.new("testinBdwatest");
return ''+str.contains('st')+
          str.contains('testing')+
          str.contains('Bdw')+
          str.contains('') +
          str.contains(MatteString.new('aaa')) +
          str.contains(MatteString.new('wat'));
