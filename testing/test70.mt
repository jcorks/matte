////Test 55
//
// Core: String (3)
<@>String = import('Matte.String');

@str = String.new("testing");
return ''+str.search('st')+
          str.search('testing')+
          str.search('aaa')+
          str.search('') +
          str.search(String.new('in'));
