//// Test81
//
// 

@a = 'abcdefgh';


@strout = '';
for(0, a->length) ::(i){
    strout = strout + a->charCodeAt(:i);
}



return strout;
