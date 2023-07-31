////Test 55
//
// Core: String (3)

@str = "testing";
return ''+str->search(key:'st')+
          str->search(key:'testing')+
          str->search(key:'aaa')+
          str->search(key:'') +
          str->search(key:'in') + (::<={
            @:results = 'testingtesting'->searchAll(key:'ti');
            @out = '';
            results->foreach(do:::(i, index) {
                out = out + index;
            });
            
            return out;
          });
          
          
