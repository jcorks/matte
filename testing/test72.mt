////Test 57
//
// Core: String (5)
@:MatteString = import(module:'Matte.Core.String');

@str = MatteString.new(from:"testinBdwatest");
return ''+str.containsAny(keys:['z', 'x', 'y'])+
          str.containsAny(keys:['f', 'f', 'a'])+
          str.containsAny(keys:['xz', '', 'ss'])+
          str.containsAny(keys:['tesst', 'BBda', 'estt'])+
          str.containsAny(keys:['test', 'Bdw', 'est'])+
          str.containsAny(keys:['ttt', MatteString.new(from:'tin'), 'eee']);


