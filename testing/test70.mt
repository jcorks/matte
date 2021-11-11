////Test 55
//
// Core: String (3)
<@>MatteString = import('Matte.Core.String');

@str = MatteString.new("testing");
return ''+str.search('st')+
          str.search('testing')+
          str.search('aaa')+
          str.search('') +
          str.search(MatteString.new('in'));
