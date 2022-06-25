//// Test81
//
// 

@a = 'abcdefgh';


@strout = '';
[0, a->length]->for(do:::(i){
    strout = strout + a->charCodeAt(index:i);
});



return strout;
