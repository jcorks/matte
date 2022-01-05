////Test 55
//
// Core: String (3)
@:MatteString = import(module:'Matte.Core.String');

@str = MatteString.new(from:"testing");
return ''+str.search(key:'st')+
          str.search(key:'testing')+
          str.search(key:'aaa')+
          str.search(key:'') +
          str.search(key:MatteString.new(from:'in'));
