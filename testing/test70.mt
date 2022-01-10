////Test 55
//
// Core: String (3)

@str = "testing";
return ''+String.search(string:str, key:'st')+
          String.search(string:str, key:'testing')+
          String.search(string:str, key:'aaa')+
          String.search(string:str, key:'') +
          String.search(string:str, key:'in');
