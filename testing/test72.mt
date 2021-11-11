////Test 57
//
// Core: String (5)
<@>MatteString = import('Matte.Core.String');

@str = MatteString.new("testinBdwatest");
return ''+str.containsAny(['z', 'x', 'y'])+
          str.containsAny(['f', 'f', 'a'])+
          str.containsAny(['xz', '', 'ss'])+
          str.containsAny(['tesst', 'BBda', 'estt'])+
          str.containsAny(['test', 'Bdw', 'est'])+
          str.containsAny(['ttt', MatteString.new('tin'), 'eee']);


