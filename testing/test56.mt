////Test 56
//
// Core: String (4)
<@>String = import('Matte.String');

@str = String.new("testinBdwatest");
return ''+str.contains('st')+
          str.contains('testing')+
          str.contains('Bdw')+
          str.contains('') +
          str.contains(String.new('aaa')) +
          str.contains(String.new('wat'));
