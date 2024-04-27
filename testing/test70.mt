////Test 55
//
// Core: String (3)

@str = "testing";
return ''+str->search(:'st')+
          str->search(:'testing')+
          str->search(:'aaa')+
          str->search(:'') +
          str->search(:'in') + (::<={
            @:results = 'testingtesting'->searchAll(:'ti');
            @out = '';
            results->foreach(::(i, index) {
                out = out + index;
            });
            
            return out;
          });
          
          
