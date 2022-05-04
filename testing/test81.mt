//// Test81
//
// 

@a = 'abcdefgh';


@strout = '';
for(in:[0, a->length], do:::(i){
    strout = strout + a->charCodeAt(index:i);
});



return strout;
