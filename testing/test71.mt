////Test 56
//
// Core: String (4)
<@>MatteString = import(module:'Matte.Core.String');

@str = MatteString.new(from:"testinBdwatest");
return ''+str.contains(key:'st')+
          str.contains(key:'testing')+
          str.contains(key:'Bdw')+
          str.contains(key:'') +
          str.contains(key:MatteString.new(from:'aaa')) +
          str.contains(key:MatteString.new(from:'wat'));
