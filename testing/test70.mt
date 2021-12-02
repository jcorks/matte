////Test 55
//
// Core: String (3)
<@>MatteString = import('Matte.Core.String');

@str = MatteString.new(from:"testing");
return ''+str.search(for:'st')+
          str.search(for:'testing')+
          str.search(for:'aaa')+
          str.search(for:'') +
          str.search(for:MatteString.new('in'));
