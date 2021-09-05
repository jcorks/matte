////Test 57
//
// Core: String (5)
<@>String = import('Matte.String');

@str = String.new("testinBdwatest");
return ''+str.containsAny(['z', 'x', 'y'])+
          str.containsAny(['f', 'f', 'a'])+
          str.containsAny(['xz', '', 'ss'])+
          str.containsAny(['tesst', 'BBda', 'estt'])+
          str.containsAny(['test', 'Bdw', 'est'])+
          str.containsAny(['ttt', String.new('tin'), 'eee']);


