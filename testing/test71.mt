////Test 56
//
// Core: String (4)
<@>MatteString = import('Matte.Core.String');

@str = MatteString.new("testinBdwatest");
return ''+str.contains('st')+
          str.contains('testing')+
          str.contains('Bdw')+
          str.contains('') +
          str.contains(MatteString.new('aaa')) +
          str.contains(MatteString.new('wat'));
